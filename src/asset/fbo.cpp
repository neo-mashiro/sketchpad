#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "core/window.h"
#include "asset/vao.h"
#include "asset/fbo.h"
#include "asset/shader.h"
#include "asset/texture.h"
#include "utils/path.h"

namespace asset {

    // optimize context switching by avoiding unnecessary binds and unbinds
    static GLuint curr_bound_renderbuffer = 0;
    static GLuint curr_bound_framebuffer = 0;

    static asset_tmp<VAO> internal_vao = nullptr;
    static asset_tmp<Shader> internal_shader = nullptr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    RBO::RBO(GLuint width, GLuint height, bool multisample) : IAsset() {
        glCreateRenderbuffers(1, &id);

        if (multisample) {
            glNamedRenderbufferStorageMultisample(id, 4, GL_DEPTH24_STENCIL8, width, height);
        }
        else {
            glNamedRenderbufferStorage(id, GL_DEPTH24_STENCIL8, width, height);
        }
    }

    RBO::~RBO() {
        glDeleteRenderbuffers(1, &id);
    }

    void RBO::Bind() const {
        if (id != curr_bound_renderbuffer) {
            glBindRenderbuffer(GL_RENDERBUFFER, id);
            curr_bound_renderbuffer = id;
        }
    }

    void RBO::Unbind() const {
        if (curr_bound_renderbuffer == id) {
            curr_bound_renderbuffer = 0;
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    FBO::FBO(GLuint width, GLuint height) : IAsset(), width(width), height(height), status(0) {
        glDisable(GL_FRAMEBUFFER_SRGB);  // important! turn off colorspace correction globally
        glCreateFramebuffers(1, &id);

        if (!internal_vao) {
            internal_vao = WrapAsset<VAO>();
        }

        if (!internal_shader) {
            internal_shader = WrapAsset<Shader>(utils::paths::shader + "core\\framebuffer.glsl");
        }
    }

    FBO::~FBO() {
        Unbind();
        glDeleteFramebuffers(1, &id);
    }

    void FBO::AddColorTexture(GLuint count, bool multisample) {
        static size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_attachments.size();

        if (n_color_buffs + count > max_color_buffs) {
            CORE_ERROR("Unable to add {0} color attachments to the framebuffer", count);
            CORE_ERROR("A framebuffer can have at most {0} color attachments", max_color_buffs);
            return;
        }

        color_attachments.reserve(n_color_buffs + count);  // allocate storage upfront

        for (GLuint i = 0; i < count; i++) {
            GLenum target = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            auto& texture = color_attachments.emplace_back(target, width, height, 1, GL_RGBA16F, 1);
            GLuint tid = texture.ID();

            static const float border[] = { 0.0f, 0.0f, 0.0f, 1.0f };

            // we cannot set any of the sampler states for multisampled textures
            if (!multisample) {
                glTextureParameteri(tid, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(tid, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(tid, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTextureParameteri(tid, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTextureParameterfv(tid, GL_TEXTURE_BORDER_COLOR, border);
            }

            glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + n_color_buffs + i, tid, 0);
        }

        SetDrawBuffers();  // all render targets are enabled for writing by default
        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::SetColorTexture(GLenum index, GLuint texture_2d) {
        static size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_attachments.size();

        CORE_ASERT(index < max_color_buffs, "Color attachment index {0} is out of range!", index);
        CORE_ASERT(index >= n_color_buffs, "Color attachment {0} is already occupied!", index);

        // texture_2d can be a multisampled texture
        glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + index, texture_2d, 0);

        SetDrawBuffers();
        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::SetColorTexture(GLenum index, GLuint texture_cubemap, GLuint face) {
        static size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;
        size_t n_color_buffs = color_attachments.size();

        CORE_ASERT(index < max_color_buffs, "Color attachment index {0} is out of range!", index);
        CORE_ASERT(index >= n_color_buffs, "Color attachment {0} is already occupied!", index);
        CORE_ASERT(face < 6, "Invalid cubemap face id, must be a number between 0 and 5!");

        if constexpr (true) {
            // some Intel drivers do not support this DSA function
            glNamedFramebufferTextureLayer(id, GL_COLOR_ATTACHMENT0 + index, texture_cubemap, 0, face);
        }
        else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture_cubemap, 0);
        }
        
        SetDrawBuffers();
        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepStTexture(bool multisample) {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        CORE_ASERT(!depst_renderbuffer, "The framebuffer already has a depth stencil renderbuffer...");
        CORE_ASERT(!depst_texture, "Only one depth stencil texture can be attached to the framebuffer...");

        // depth stencil textures are meant to be filtered anyway, it doesn't make sense to use a depth
        // stencil texture for MSAA because filtering on multisampled textures is not allowed by OpenGL.
        if (multisample) {
            CORE_ERROR("Multisampled depth stencil texture is not supported, it is a waste of memory!");
            CORE_ERROR("If you need MSAA, please add a multisampled renderbuffer (RBO) instead...");
            return;
        }

        // depth and stencil values are combined in a single immutable-format texture
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value
        depst_texture = WrapAsset<Texture>(GL_TEXTURE_2D, width, height, 1, GL_DEPTH24_STENCIL8, 1);
        glTextureParameteri(depst_texture->ID(), GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

        GLint immutable_format;
        glGetTextureParameteriv(depst_texture->ID(), GL_TEXTURE_IMMUTABLE_FORMAT, &immutable_format);
        CORE_ASERT(immutable_format == GL_TRUE, "Unable to attach an immutable depth stencil texture...");

        // to access the stencil values in GLSL, we need a separate texture view
        stencil_view = WrapAsset<TexView>(*depst_texture);
        stencil_view->SetView(GL_TEXTURE_2D, 0, 1, 0, 1);
        glTextureParameteri(stencil_view->ID(), GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);

        glNamedFramebufferTexture(id, GL_DEPTH_STENCIL_ATTACHMENT, depst_texture->ID(), 0);
        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepStRenderBuffer(bool multisample) {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        CORE_ASERT(!depst_texture, "The framebuffer already has a depth stencil texture...");
        CORE_ASERT(!depst_renderbuffer, "Only one depth stencil renderbuffer can be attached to the framebuffer...");

        // depth and stencil values are combined in a single renderbuffer (RBO)
        // each 32-bit pixel contains 24 bits of depth value and 8 bits of stencil value

        depst_renderbuffer = WrapAsset<RBO>(width, height, multisample);
        depst_renderbuffer->Bind();
        glNamedFramebufferRenderbuffer(id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depst_renderbuffer->ID());

        // the depth and stencil buffer in the form of a renderbuffer is write-only
        // we can't read it later so there's no need to create a stencil texture view

        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    void FBO::AddDepthCubemap() {
        // a framebuffer can only have one depth stencil buffer, either as a texture or a renderbuffer
        CORE_ASERT(!depst_renderbuffer, "The framebuffer already has a depth stencil renderbuffer...");
        CORE_ASERT(!depst_texture, "Only one depth stencil texture can be attached to the framebuffer...");

        // for omni-directional shadow mapping, we only need a cubemap depth texture of high precision
        // we can obtain the best precision using `GL_DEPTH_COMPONENT32F`, but it's mostly an overkill
        // and quite slow in performance. In practice, people commonly use `GL_DEPTH_COMPONENT24/16`

        depst_texture = WrapAsset<Texture>(GL_TEXTURE_CUBE_MAP, width, height, 6, GL_DEPTH_COMPONENT24, 1);
        GLuint tid = depst_texture->ID();

        glTextureParameteri(tid, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
        glTextureParameteri(tid, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(tid, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(tid, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tid, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tid, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, tid, 0);
        const GLenum null[] = { GL_NONE };
        glNamedFramebufferReadBuffer(id, GL_NONE);
        glNamedFramebufferDrawBuffers(id, 1, null);

        status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    }

    const Texture& FBO::GetColorTexture(GLenum index) const {
        CORE_ASERT(index < color_attachments.size(), "Invalid color attachment index: {0}", index);
        return color_attachments[index];
    }

    const Texture& FBO::GetDepthTexture() const {
        CORE_ASERT(depst_texture, "The framebuffer does not have a depth texture...");
        return *depst_texture;
    }

    const TexView& FBO::GetStencilTexView() const {
        CORE_ASERT(stencil_view, "The framebuffer does not have a stencil texture view...");
        return *stencil_view;
    }

    void FBO::Bind() const {
        if (id != curr_bound_framebuffer) {
            CORE_ASERT(status == GL_FRAMEBUFFER_COMPLETE, "Incomplete framebuffer status: {0}", status);
            if (depst_renderbuffer) {
                depst_renderbuffer->Bind();
            }
            glBindFramebuffer(GL_FRAMEBUFFER, id);
            curr_bound_framebuffer = id;
        }
    }

    void FBO::Unbind() const {
        if (curr_bound_framebuffer == id) {
            curr_bound_framebuffer = 0;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void FBO::SetDrawBuffer(GLuint index) const {
        // this function only enables a single color attachment for writing
        CORE_ASERT(index < color_attachments.size(), "Color buffer index out of bound!");
        const GLenum buffers[] = { GL_COLOR_ATTACHMENT0 + index };
        glNamedFramebufferDrawBuffers(id, 1, buffers);
    }

    void FBO::SetDrawBuffers(std::vector<GLuint> indices) const {
        // this function enables the input list of color attachments for writing
        size_t n_buffs = color_attachments.size();
        size_t n_index = indices.size();
        GLenum* buffers = new GLenum[n_index];

        for (GLenum i = 0; i < n_index; i++) {
            // the `layout(location = i) out` variable will write to this attachment
            GLuint index = indices[i];
            CORE_ASERT(index < n_buffs, "Color buffer index {0} out of bound!", index);
            *(buffers + i) = GL_COLOR_ATTACHMENT0 + index;
        }

        glNamedFramebufferDrawBuffers(id, n_index, buffers);
        delete[] buffers;
    }

    void FBO::SetDrawBuffers() const {
        // enable all color attachments for writing
        if (size_t n = color_attachments.size(); n > 0) {
            GLenum* attachments = new GLenum[n];

            for (GLenum i = 0; i < n; i++) {
                *(attachments + i) = GL_COLOR_ATTACHMENT0 + i;
            }

            glNamedFramebufferDrawBuffers(id, n, attachments);
            delete[] attachments;
        }
    }

    void FBO::Draw(GLint index) const {
        internal_vao->Bind();
        internal_shader->Bind();

        // subroutine indexes are explicitly specified in the shader, see "framebuffer.glsl"
        GLuint subroutine_index = 0;

        // visualize one of the color attachments
        if (index >= 0 && index < color_attachments.size()) {
            subroutine_index = 0;
            color_attachments[index].Bind(0);
        }
        // visualize the linearized depth buffer
        else if (index == -1) {
            subroutine_index = 1;
            if (depst_texture != nullptr) {
                depst_texture->Bind(0);
            }
            else {
                CORE_ERROR("Unable to visualize the depth buffer, depth texture not available!");
            }
        }
        // visualize the stencil buffer
        else if (index == -2) {
            subroutine_index = 2;
            if (stencil_view != nullptr) {
                stencil_view->Bind(1);  // stencil view uses texture unit 1
            }
            else {
                CORE_ERROR("Unable to visualize the stencil buffer, stencil view not available!");
            }
        }
        else {
            CORE_ERROR("Buffer index {0} is not valid in the framebuffer!", index);
            CORE_ERROR("Valid indices: 0-{0} (colors), -1 (depth), -2 (stencil)", color_attachments.size() - 1);
        }

        // subroutine states are never preserved, so we must reset the subroutine uniform every
        // single time (fragment shader won't remember the subroutine uniform's previous value)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);

        glDrawArrays(GL_TRIANGLES, 0, 3);  // bufferless quad rendering
    }

    void FBO::Clear(GLint index) const {
        const GLfloat clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        const GLfloat clear_depth = 1.0f;
        const GLint clear_stencil = 0;

        // a framebuffer always has a depth buffer, a stencil buffer and all color buffers,
        // an empty buffer just doesn't have any textures attached to it, but the buffer is
        // still there. It's ok to clear a buffer even if there's no textures attached

        const size_t max_color_buffs = core::Application::GetInstance().gl_max_color_buffs;

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

    void FBO::Clear() const {
        for (int i = 0; i < color_attachments.size(); i++) {
            Clear(i);
        }

        Clear(-1);
        Clear(-2);
    }

    void FBO::CopyColor(const FBO& fr, GLuint fr_idx, const FBO& to, GLuint to_idx) {
        CORE_ASERT(fr_idx < fr.color_attachments.size(), "Color buffer index {0} out of bound...", fr_idx);
        CORE_ASERT(to_idx < to.color_attachments.size(), "Color buffer index {0} out of bound...", to_idx);

        // if the source and target rectangle areas differ in size, interpolation will be applied
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;

        glNamedFramebufferReadBuffer(fr.id, GL_COLOR_ATTACHMENT0 + fr_idx);
        glNamedFramebufferDrawBuffer(to.id, GL_COLOR_ATTACHMENT0 + to_idx);
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::CopyDepth(const FBO& fr, const FBO& to) {
        // make sure that GL_FRAMEBUFFER_SRGB is globally disabled when calling this function!
        // if colorspace correction is enabled, depth values will be gamma encoded during blits...
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void FBO::CopyStencil(const FBO& fr, const FBO& to) {
        // make sure that GL_FRAMEBUFFER_SRGB is globally disabled when calling this function!
        // if colorspace correction is enabled, stencil values will be gamma encoded during blits...
        GLuint fw = fr.width, fh = fr.height;
        GLuint tw = to.width, th = to.height;
        glBlitNamedFramebuffer(fr.id, to.id, 0, 0, fw, fh, 0, 0, tw, th, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

}
