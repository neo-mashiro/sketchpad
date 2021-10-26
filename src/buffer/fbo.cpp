#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "core/window.h"
#include "buffer/rbo.h"
#include "buffer/fbo.h"
#include "buffer/texture.h"
#include "components/mesh.h"
#include "components/shader.h"
#include "components/material.h"
#include "utils/path.h"

using namespace core;
using namespace components;

namespace buffer {

    FBO::FBO(GLuint width, GLuint height) : Buffer(), width(width), height(height), status(0) {
        // globally turn off colorspace correction
        glDisable(GL_FRAMEBUFFER_SRGB);

        // framebuffer size (texture size) must be less than or equal to the window size
        if (width > Window::width || height > Window::height) {
            CORE_ERROR("Framebuffer size cannot exceed the window size!");
            return;
        }

        glCreateFramebuffers(1, &id);

        // attach a virtual mesh, virtual material and virtual shader
        virtual_mesh     = std::make_unique<Mesh>(Primitive::Quad2D);
        virtual_material = std::make_unique<Material>();
        virtual_shader   = std::make_unique<Shader>(SHADER + "fullscreen.glsl");
    }

    FBO::~FBO() {
        CORE_WARN("Destroying framebuffer: {0}...", id);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &id);
    }

    FBO::FBO(FBO&& other) noexcept {
        *this = std::move(other);
    }

    FBO& FBO::operator=(FBO&& other) noexcept {
        if (this != &other) {
            color_textures.clear();
            stencil_view = 0;
            depst_texture = nullptr;
            depst_renderbuffer = nullptr;

            virtual_mesh = nullptr;
            virtual_shader = nullptr;
            virtual_material = nullptr;

            std::swap(id, other.id);
            std::swap(width, other.width);
            std::swap(height, other.height);
            std::swap(status, other.status);

            std::swap(color_textures, other.color_textures);
            std::swap(depst_texture, other.depst_texture);
            std::swap(depst_renderbuffer, other.depst_renderbuffer);
            std::swap(stencil_view, other.stencil_view);

            std::swap(virtual_mesh, other.virtual_mesh);
            std::swap(virtual_shader, other.virtual_shader);
            std::swap(virtual_material, other.virtual_material);
        }

        return *this;
    }

    void FBO::AddColorTexture(GLuint count) {
        size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_textures.size();

        if (n_color_buffs + count > max_color_buffs) {
            CORE_ERROR("Unable to add {0} color attachments to the framebuffer", count);
            CORE_ERROR("A framebuffer can have at most {0} color attachments", max_color_buffs);
            return;
        }

        for (GLuint i = 0; i < count; i++) {
            color_textures.emplace_back(GL_TEXTURE_2D, width, height, GL_RGBA16F);
            auto& texture = color_textures.back();
            glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + n_color_buffs + i, texture.GetID(), 0);
        }

        // enable multiple render targets
        if (size_t n = color_textures.size(); n > 0) {
            GLenum* attachments = new GLenum[n];

            for (GLenum i = 0; i < n; i++) {
                *(attachments + i) = GL_COLOR_ATTACHMENT0 + i;
            }

            glNamedFramebufferDrawBuffers(id, n, attachments);
            delete[] attachments;
        }

        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepStTexture() {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        if (depst_renderbuffer != nullptr) {
            CORE_ERROR("This framebuffer already has a depth stencil renderbuffer...");
            return;
        }

        // depth and stencil values are combined in a single immutable-format texture
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value
        depst_texture = std::make_unique<Texture>(GL_TEXTURE_2D, width, height, GL_DEPTH24_STENCIL8);
        glTextureParameteri(depst_texture->GetID(), GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

        GLint immutable_format;
        glGetTextureParameteriv(depst_texture->GetID(), GL_TEXTURE_IMMUTABLE_FORMAT, &immutable_format);

        if (immutable_format != GL_TRUE) {
            CORE_ERROR("Unable to attach an immutable depth stencil texture...");
            return;
        }

        // to access the stencil values in GLSL, we need a separate texture view
        stencil_view = std::make_unique<TexView>(*depst_texture);
        glTextureParameteri(stencil_view->GetID(), GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);

        glNamedFramebufferTexture(id, GL_DEPTH_STENCIL_ATTACHMENT, depst_texture->GetID(), 0);

        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepStRenderBuffer() {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        if (depst_texture != nullptr) {
            CORE_ERROR("This framebuffer already has a depth stencil texture...");
            return;
        }

        // depth and stencil values are combined in a single render buffer (RBO)
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value

        depst_renderbuffer = std::make_unique<RBO>(width, height);
        depst_renderbuffer->Bind();
        glNamedFramebufferRenderbuffer(id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depst_renderbuffer->GetID());

        // the depth and stencil buffer in the form of a renderbuffer is write-only
        // we won't access it later so there's no need to create a stencil texture view

        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    const Texture& FBO::GetColorTexture(GLenum index) const {
        CORE_ASERT(index < color_textures.size(), "Invalid color attachment index: {0}", index);
        return color_textures[index];
    }

    const Texture& FBO::GetDepthTexture() const {
        CORE_ASERT(depst_texture, "This framebuffer does not have a depth texture...");
        return *depst_texture;
    }

    const TexView& FBO::GetStencilTexView() const {
        CORE_ASERT(stencil_view, "This framebuffer does not have a stencil texture view...");
        return *stencil_view;
    }

    Material& FBO::GetVirtualMaterial() {
        return *virtual_material;
    }

    void FBO::Bind() const {
        CORE_ASERT(status == GL_FRAMEBUFFER_COMPLETE, "Incomplete framebuffer status: {0}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }

    void FBO::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FBO::PostProcessDraw() const {
        size_t n = color_textures.size();

        if (virtual_material->Bind()) {
            // bind all color textures to texture units 0 ~ n-1
            for (size_t i = 0; i < n; i++) {
                color_textures[i].Bind(i);
            }

            // bind the depth component to texture unit n
            if (depst_texture != nullptr) {
                depst_texture->Bind(n);
            }

            // bind the stencil component to texture unit n+1
            if (stencil_view != nullptr) {
                stencil_view->Bind(n + 1);
            }

            // draw the quad mesh while the virtual material is bound
            virtual_mesh->Draw();

            // unbind all textures after the draw call
            for (size_t i = 0; i < n + 2; i++) {
                glBindTextureUnit(i, 0);
            }

            virtual_material->Unbind();
        }
    }

    void FBO::DebugDraw(GLint index) const {
        virtual_shader->Bind();

        // subroutine indexes are explicitly specified in the shader, see "fullscreen.glsl"
        static GLuint subroutine_index = 0;

        // visualize one of the color attachments
        if (index >= 0 && index < color_textures.size()) {
            subroutine_index = 0;
            color_textures[index].Bind(0);
        }
        // visualize the linearized depth buffer
        else if (index == -1) {
            subroutine_index = 1;
            if (depst_texture != nullptr) {
                depst_texture->Bind(0);
            }
            else {
                CORE_ERROR("Unable to visualize the depth buffer, depth texture not found!");
            }
        }
        // visualize the stencil buffer
        else if (index == -2) {
            subroutine_index = 2;
            if (stencil_view != nullptr) {
                stencil_view->Bind(1);  // stencil view uses texture unit 1
            }
            else {
                CORE_ERROR("Unable to visualize the stencil buffer, stencil view not found!");
            }
        }
        else {
            CORE_ERROR("Buffer index {0} is not valid in the framebuffer!", index);
            CORE_ERROR("Valid indices: 0-{0} (colors), -1 (depth), -2 (stencil)", color_textures.size() - 1);
        }

        // subroutine states are never preserved, so we must reset the subroutine uniform every
        // single time (fragment shader won't remember the subroutine uniform's previous value)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);

        virtual_mesh->Draw();
        virtual_shader->Unbind();
    }

    void FBO::Clear(GLint index) const {
        static GLfloat clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static GLfloat clear_depth = 1.0f;
        static GLint clear_stencil = 0;

        // clear one of the color attachments
        if (index >= 0 && index < color_textures.size()) {
            glClearNamedFramebufferfv(id, GL_COLOR, index, clear_color);
        }
        // clear the depth buffer
        else if (index == -1 && depst_texture) {
            glClearNamedFramebufferfv(id, GL_DEPTH, 0, &clear_depth);
        }
        // clear the stencil buffer
        else if (index == -2 && stencil_view) {
            glClearNamedFramebufferiv(id, GL_STENCIL, 0, &clear_stencil);
        }
        else {
            CORE_ERROR("Buffer index {0} is not valid in the framebuffer!", index);
            CORE_ERROR("Valid indices: 0-{0} (colors), -1 (depth), -2 (stencil)", color_textures.size() - 1);
        }
    }

    void FBO::TransferColor(const FBO& fr, GLuint fr_idx, const FBO& to, GLuint to_idx) {
        CORE_ASERT(fr_idx < fr.color_textures.size(), "Color buffer index {0} out of bound...", fr_idx);
        CORE_ASERT(to_idx < to.color_textures.size(), "Color buffer index {0} out of bound...", to_idx);

        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;

        glNamedFramebufferReadBuffer(fr.id, GL_COLOR_ATTACHMENT0 + fr_idx);
        glNamedFramebufferDrawBuffer(to.id, GL_COLOR_ATTACHMENT0 + to_idx);
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::TransferDepth(const FBO& fr, const FBO& to) {
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::TransferStencil(const FBO& fr, const FBO& to) {
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

}
