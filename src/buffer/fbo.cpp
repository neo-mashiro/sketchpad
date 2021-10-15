#include "pch.h"

#include "core/log.h"
#include "core/window.h"
#include "buffer/fbo.h"
#include "components/mesh.h"
#include "components/shader.h"
#include "components/material.h"
#include "utils/path.h"

using namespace core;
using namespace components;

namespace buffer {

    static GLint max_color_attachments { -1 };
    static GLint max_draw_buffers { -1 };

    FBO::FBO(GLuint n_color_buff, GLuint width, GLuint height) : Buffer(), width(width), height(height) {
        // globally turn off colorspace correction
        glDisable(GL_FRAMEBUFFER_SRGB);

        // framebuffer size (texture size) must be less than or equal to the window size
        if (width > Window::width || height > Window::height) {
            CORE_ERROR("Framebuffer size cannot exceed the window size!");
            return;
        }

        if (max_color_attachments < 0) glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
        if (max_draw_buffers < 0) glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

        if (n_color_buff > std::min(max_color_attachments, max_draw_buffers)) {
            CORE_ERROR("A framebuffer can only have up to {0} color attachments", max_color_attachments);
            CORE_ERROR("The maximum number of draw buffers supported is {0}", max_draw_buffers);
            return;
        }
        
        glGenFramebuffers(1, &id);
        glBindFramebuffer(GL_FRAMEBUFFER, id);

        // allocate GPU memory for all color attachments (buffers)
        color_textures.clear();
        for (GLenum i = 0; i < n_color_buff; i++) {
            AttachColorBuffer(i);
        }

        // the depth and stencil attachment are combined in a single texture
        AttachDepthStencilBuffer();

        // enable multiple render targets
        EnableMRT();
        
        // framebuffer completeness check
        if (CheckStatus()) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else {
            CORE_ERROR("Failed to construct the framebuffer...");
            std::cin.get();
            exit(EXIT_FAILURE);
        }
        
        // framebuffer is complete, attach a virtual mesh, material and a debug shader
        virtual_mesh     = std::make_unique<Mesh>(Primitive::Quad2D);
        virtual_material = std::make_unique<Material>();
        debug_shader     = LoadAsset<Shader>(SHADER + "fullscreen.glsl");
    }

    FBO::~FBO() {
        CORE_WARN("Destroying framebuffer with attachments (id = {0})!", id);

        for (auto& texture : color_textures) {
            glDeleteTextures(1, &texture);
        }

        glDeleteTextures(1, &depth_stencil_texture);
        glDeleteTextures(1, &stencil_view);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &id);
    }

    void FBO::AttachColorBuffer(GLenum idx) {
        GLuint texture_id { 0 };
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // the high-precision color format "GL_RGBA16F" has 16-bit float per component, it can help
        // us preserve the source data more accurately even at a high window resolution. We can also
        // use the "GL_RGBA32F" format but I think it's overkill for this demo......
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, GL_TEXTURE_2D, texture_id, 0);

        color_textures.push_back(texture_id);
    }

    void FBO::AttachDepthStencilBuffer() {
        // depth and stencil values are combined in a single immutable-format texture
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value
        glGenTextures(1, &depth_stencil_texture);
        glBindTexture(GL_TEXTURE_2D, depth_stencil_texture);
        
        // allocate memory as an immutable-format texture
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, width, height);

        GLint param = 0;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &param);

        if (param != GL_TRUE) {
            CORE_ERROR("Cannot allocate storage as an immutable-format texture...");
            return;
        }

        // to access the stencil buffer in GLSL, we need a separate texture view
        // https://stackoverflow.com/questions/27535727
        GLuint stencil_view;
        glGenTextures(1, &stencil_view);
        glTextureView(stencil_view, GL_TEXTURE_2D, depth_stencil_texture, GL_DEPTH24_STENCIL8, 0, 1, 0, 1);

        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_stencil_texture, 0);
    }

    void FBO::EnableMRT() const {
        // enable multiple render targets
        size_t n_color_buff = color_textures.size();
        GLenum* attachments = new GLenum[n_color_buff];

        for (GLenum i = 0; i < n_color_buff; i++) {
            *(attachments + i) = GL_COLOR_ATTACHMENT0 + i;
        }

        glDrawBuffers(n_color_buff, attachments);
        delete[] attachments;
    }

    bool FBO::CheckStatus() const {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::string status_str;
            switch (status) {
                case 0x8219: status_str = "GL_FRAMEBUFFER_UNDEFINED";                     break;
                case 0x8CD6: status_str = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";         break;
                case 0x8CD7: status_str = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
                case 0x8CDB: status_str = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";        break;
                case 0x8CDC: status_str = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";        break;
                case 0x8CDD: status_str = "GL_FRAMEBUFFER_UNSUPPORTED";                   break;
                case 0x8D56: status_str = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";        break;
                case 0x8DA8: status_str = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";      break;
                default:     status_str = "Unknown framebuffer status";
            }

            CORE_ERROR("The active framebuffer is not complete (status = {0})", status_str);
            return false;
        }

        return true;
    }

    void FBO::Bind() const {
        // this function always binds to both the read and write target, there's no need to introduce
        // a target parameter. If you must bind to one of the read or write buffers, use the DSA call.
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }

    void FBO::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FBO::BindBuffer(GLint buffer_id, GLuint unit) const {
        if (buffer_id >= 0 && buffer_id < color_textures.size()) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, color_textures[buffer_id]);
        }
        else if (buffer_id == -1) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, depth_stencil_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
        }
        else if (buffer_id == -2) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, stencil_view);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
        }
        else {
            CORE_ERROR("Buffer id {0} is not found in the framebuffer!", buffer_id);
            CORE_ERROR("Valid buffer ids: 0-{0} (colors), -1 (depth), -2 (stencil)", color_textures.size() - 1);
        }
    }

    void FBO::UnbindBuffer(GLuint unit) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Material* const FBO::GetVirtualMaterial() {
        return virtual_material.get();
    }

    void FBO::PostProcessDraw() const {
        size_t n = color_textures.size();

        if (virtual_material->Bind()) {
            // bind all color textures to texture units 0 ~ n-1
            for (size_t i = 0; i < n; i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, color_textures[i]);
            }

            // bind the depth component to texture unit n
            glActiveTexture(GL_TEXTURE0 + n);
            glBindTexture(GL_TEXTURE_2D, depth_stencil_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

            // bind the stencil component to texture unit n+1
            glActiveTexture(GL_TEXTURE0 + n + 1);
            glBindTexture(GL_TEXTURE_2D, stencil_view);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);

            // draw the fullscreen quad while the virtual material is bound
            virtual_mesh->Draw();

            // unbind all textures after the draw call
            for (size_t i = 0; i < n + 2; i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            virtual_material->Unbind();
        }
    }

    void FBO::DebugDraw(GLint buffer_id) const {
        debug_shader->Bind();

        // subroutine indexes are explicitly specified in the shader, see "fullscreen.glsl"
        static GLuint subroutine_index = 0;

        // draw one of the attached color textures
        if (buffer_id >= 0 && buffer_id < color_textures.size()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, color_textures[buffer_id]);
            subroutine_index = 0;
        }
        // visualize the linearized depth buffer
        else if (buffer_id == -1) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depth_stencil_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
            subroutine_index = 1;
        }
        // visualize the stencil buffer
        else if (buffer_id == -2) {
            glActiveTexture(GL_TEXTURE1);  // stencil uses texture unit 1
            glBindTexture(GL_TEXTURE_2D, stencil_view);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
            subroutine_index = 2;
        }
        else {
            CORE_ERROR("Buffer id {0} is not found in the framebuffer!", buffer_id);
            CORE_ERROR("Valid buffer ids: 0-{0} (colors), -1 (depth), -2 (stencil)", color_textures.size() - 1);
        }

        // subroutine states are never preserved, so we must reset the subroutine uniform every
        // single time (fragment shader won't remember the subroutine uniform's previous value)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);

        virtual_mesh->Draw();
        debug_shader->Unbind();
    }

    void FBO::Clear(GLint buffer_id) const {
        static GLfloat clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static GLfloat clear_depth = 1.0f;
        static GLint clear_stencil = 0;

        // clear one of the attached color textures
        if (buffer_id >= 0 && buffer_id < color_textures.size()) {
            glClearNamedFramebufferfv(id, GL_COLOR, buffer_id, clear_color);
        }
        // clear the depth buffer
        else if (buffer_id == -1) {
            glClearNamedFramebufferfv(id, GL_DEPTH, 0, &clear_depth);
        }
        // clear the stencil buffer
        else if (buffer_id == -2) {
            glClearNamedFramebufferiv(id, GL_STENCIL, 0, &clear_stencil);
        }
        else {
            CORE_ERROR("Buffer id {0} is not found in the framebuffer!", buffer_id);
            CORE_ERROR("Valid buffer ids: 0-{0} (colors), -1 (depth), -2 (stencil)", color_textures.size() - 1);
        }
    }

    void FBO::TransferColor(FBO* in, GLuint in_buff, FBO* out, GLuint out_buff) {
        CORE_ASERT(in_buff < in->color_textures.size(), "Color buffer id {0} is out of bound...", in_buff);
        CORE_ASERT(out_buff < out->color_textures.size(), "Color buffer id {0} is out of bound...", out_buff);

        GLuint iw = in->width, ih = in->height;
        GLuint ow = out->width, oh = out->height;

        glNamedFramebufferReadBuffer(in->id, GL_COLOR_ATTACHMENT0 + in_buff);
        glNamedFramebufferDrawBuffer(out->id, GL_COLOR_ATTACHMENT0 + out_buff);
        glBlitNamedFramebuffer(in->id, out->id, 0, 0, iw, ih, 0, 0, ow, oh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::TransferDepth(FBO* in, FBO* out) {
        GLuint iw = in->width, ih = in->height;
        GLuint ow = out->width, oh = out->height;
        glBlitNamedFramebuffer(in->id, out->id, 0, 0, iw, ih, 0, 0, ow, oh, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::TransferStencil(FBO* in, FBO* out) {
        GLuint iw = in->width, ih = in->height;
        GLuint ow = out->width, oh = out->height;
        glBlitNamedFramebuffer(in->id, out->id, 0, 0, iw, ih, 0, 0, ow, oh, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

}
