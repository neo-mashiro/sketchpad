#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "core/window.h"
#include "buffer/vao.h"
#include "buffer/rbo.h"
#include "buffer/fbo.h"
#include "buffer/texture.h"
#include "components/shader.h"
#include "utils/filesystem.h"

using namespace core;
using namespace components;

namespace buffer {

    FBO::FBO(GLuint width, GLuint height) : Buffer(), width(width), height(height), status(0) {
        // important! turn off colorspace correction globally
        glDisable(GL_FRAMEBUFFER_SRGB);

        // framebuffer size (texture size) doesn't have to be less than the window size
        // there are cases that we would want to render to a texture of arbitrary shape

        // if (width > Window::width || height > Window::height) {
        //     CORE_ERROR("Framebuffer size cannot exceed the window size!");
        //     return;
        // }

        glCreateFramebuffers(1, &id);

        // attach a static dummy VAO and debug shader for debug drawing
        if (debug_vao == nullptr) {
            debug_vao = std::make_unique<VAO>();
        }
        if (debug_shader == nullptr) {
            debug_shader = std::make_unique<Shader>(utils::paths::shaders + "fullscreen.glsl");
        }
    }

    FBO::~FBO() {
        CORE_WARN("Destroying framebuffer (id = {0})...", id);
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

            std::swap(id, other.id);
            std::swap(width, other.width);
            std::swap(height, other.height);
            std::swap(status, other.status);

            std::swap(color_textures, other.color_textures);
            std::swap(depst_texture, other.depst_texture);
            std::swap(depst_renderbuffer, other.depst_renderbuffer);
            std::swap(stencil_view, other.stencil_view);
        }

        return *this;
    }

    void FBO::AddColorTexture(GLuint count, bool multisample) {
        size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_textures.size();

        if (n_color_buffs + count > max_color_buffs) {
            CORE_ERROR("Unable to add {0} color attachments to the framebuffer", count);
            CORE_ERROR("A framebuffer can have at most {0} color attachments", max_color_buffs);
            return;
        }

        static const float border[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        for (GLuint i = 0; i < count; i++) {
            GLenum texture_target = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            color_textures.emplace_back(texture_target, width, height, GL_RGBA16F, 1, multisample);

            auto& texture = color_textures.back();
            GLuint tid = texture.GetID();

            glTextureParameteri(tid, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(tid, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(tid, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(tid, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTextureParameterfv(tid, GL_TEXTURE_BORDER_COLOR, border);
            glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + n_color_buffs + i, tid, 0);
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

    void FBO::SetColorTexture(GLenum index, GLuint cubemap_texture, GLuint face) {
        size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_textures.size();

        CORE_ASERT(index < max_color_buffs, "Color attachment index {0} is out of range!", index);
        CORE_ASERT(index >= n_color_buffs, "Color attachment {0} is already occupied!", index);
        CORE_ASERT(face < 6, "Invalid cubemap face id, must be a number between 0 and 5!");

        glNamedFramebufferTextureLayer(id, GL_COLOR_ATTACHMENT0 + index, cubemap_texture, 0, face);
        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepStTexture(bool multisample) {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        if (depst_renderbuffer != nullptr) {
            CORE_ERROR("This framebuffer already has a depth stencil renderbuffer...");
            return;
        }

        // depth stencil textures are meant to be filtered anyway, it doesn't make sense to use a depth
        // stencil texture for MSAA because filtering on multisampled textures is not allowed by OpenGL.
        if (multisample) {
            CORE_ERROR("Multisampled depth stencil texture is not supported, it is a waste of memory!");
            CORE_ERROR("If you need MSAA, please add a multisampled renderbuffer (RBO) instead...");
            return;
        }

        // depth and stencil values are combined in a single immutable-format texture
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value
        depst_texture = std::make_unique<Texture>(GL_TEXTURE_2D, width, height, GL_DEPTH24_STENCIL8, 1);
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

    void FBO::AddDepStRenderBuffer(bool multisample) {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        if (depst_texture != nullptr) {
            CORE_ERROR("This framebuffer already has a depth stencil texture...");
            return;
        }

        // depth and stencil values are combined in a single render buffer (RBO)
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value

        depst_renderbuffer = std::make_unique<RBO>(width, height, multisample);
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

    void FBO::Bind() const {
        CORE_ASERT(status == GL_FRAMEBUFFER_COMPLETE, "Incomplete framebuffer status: {0}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }

    void FBO::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FBO::SetDrawBuffer(GLuint index) const {
        CORE_ASERT(index < color_textures.size(), "Color buffer index out of bound!");
        const GLenum buffers[] = { GL_COLOR_ATTACHMENT0 + index };
        glNamedFramebufferDrawBuffers(id, 1, buffers);
    }

    void FBO::SetDrawBuffers(std::vector<GLuint> indices) const {
        size_t n_buffs = color_textures.size();
        size_t n_index = indices.size();
        GLenum* buffers = new GLenum[n_index];

        for (GLenum i = 0; i < n_index; i++) {
            GLuint index = indices[i];
            CORE_ASERT(index < n_buffs, "Color buffer index {0} out of bound!", index);
            // the `layout(location = i) out` variable will write to this attachment
            *(buffers + i) = GL_COLOR_ATTACHMENT0 + index;
        }

        glNamedFramebufferDrawBuffers(id, n_index, buffers);
        delete[] buffers;
    }

    void FBO::Draw(GLint index) const {
        debug_shader->Bind();
        debug_vao->Bind();

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

        glDrawArrays(GL_TRIANGLES, 0, 3);
        debug_shader->Unbind();
    }

    void FBO::Clear(GLint index) const {
        static GLfloat clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static GLfloat clear_depth = 1.0f;
        static GLint clear_stencil = 0;

        // a framebuffer always has a depth buffer, a stencil buffer and all color buffers,
        // an empty one just doesn't have any textures attached to it, but all buffers are
        // still there. It's ok to clear a buffer even if there's no textures attached, we
        // don't need to check `index < color_textures.size()` or `depst_texture != nullptr`

        static size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;

        // clear one of the color attachments
        if (index >= 0 && index < max_color_buffs) {
            glClearNamedFramebufferfv(id, GL_COLOR, index, clear_color);
        }
        // clear the depth buffer
        else if (index == -1) {
            glClearNamedFramebufferfv(id, GL_DEPTH, 0, &clear_depth);
        }
        // clear the stencil buffer
        else if (index == -2) {
            glClearNamedFramebufferiv(id, GL_STENCIL, 0, &clear_stencil);
        }
        else {
            CORE_ERROR("Buffer index {0} is not valid in the framebuffer!", index);
            CORE_ERROR("Valid indices: 0-{0} (colors), -1 (depth), -2 (stencil)", max_color_buffs - 1);
        }
    }

    void FBO::ClearAll() const {
        for (int i = 0; i < color_textures.size(); i++) {
            Clear(i);
        }
        Clear(-1);
        Clear(-2);
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
        // make sure that GL_FRAMEBUFFER_SRGB is globally disabled when calling this function!
        // if colorspace correction is enabled, depth values will be gamma encoded during blits...
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::TransferStencil(const FBO& fr, const FBO& to) {
        // make sure that GL_FRAMEBUFFER_SRGB is globally disabled when calling this function!
        // if colorspace correction is enabled, stencil values will be gamma encoded during blits...
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

}
