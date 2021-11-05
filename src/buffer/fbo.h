#pragma once

#include <memory>
#include <vector>
#include "buffer/buffer.h"
#include "components/component.h"
#include "components/shader.h"

namespace buffer {

    /* for safety, by default we will disable the implicit colorspace correction (silently performed
       by the hardware), so that any fragment shader output will be written to the framebuffer AS IS.
       the fragment shader is free to decide what colorspace it wants to work in, which the user must
       be well aware of. In specific, gamma correction has to be done explicitly within the fragment
       shader, rather than relying on the hardware. In particular, if you need to work with blending
       (which is expected to interact with sRGB images) with framebuffers, make sure to linearize the
       sRGB color first, then do the blending in linear RGB space, and finally convert back to sRGB.

       note that it's safe to use pointers to a `Mesh` or `Material` here because framebuffers are not
       registered in the ECS pool, but in other classes, you should never rely on such a pointer since
       they are volatile (recall that the ECS pool is free to rearrange memory address as it sees fit).
       currently, we do not support multisample FBOs, MSAA must be applied on the default framebuffer.

       # debug draw a single buffer

       user-defined FBOs are mostly used to hold temporary screen-space data in an intermediate pass
       where the main focus is to render intermediate results into the attached textures. Normally, we
       don't need to draw them directly, but sometimes we may want to visualize the contents of these
       temporary buffer for debugging purposes. The `Draw()` function does just that, which uses a
       preset debug shader (bufferless rendering).

       # clear the buffers

       users can use the `Clear()` function to clear a certain texture or buffer, or the `ClearAll()`
       function to clear all textures in one go. You should always use them to clean up a user-defined
       framebuffer, which is guaranteed to be filled with clean zeros after clearing. There is another
       function `Renderer::Clear()` which should only be used on the default framebuffer, do not use
       it on custom FBOs because it uses deep blue as the clear color, rather than black.
    */

    using namespace components;

    // forward declaration
    class VAO;
    class RBO;
    class Texture;
    class TexView;

    class FBO : public Buffer {
      private:
        GLenum status;
        GLuint width, height;

        std::vector<Texture>     color_textures;      // the vector of color attachments
        std::unique_ptr<RBO>     depst_renderbuffer;  // depth & stencil as a single renderbuffer
        std::unique_ptr<Texture> depst_texture;       // depth & stencil as a single texture
        std::unique_ptr<TexView> stencil_view;        // temporary stencil texture view

        // bufferless rendering of the fullscreen quad
        std::unique_ptr<VAO>    debug_vao;
        std::unique_ptr<Shader> debug_shader;

      public:
        FBO() = default;
        FBO(GLuint width, GLuint height);
        ~FBO();

        FBO(const FBO&) = delete;
        FBO& operator=(const FBO&) = delete;

        FBO(FBO&& other) noexcept;
        FBO& operator=(FBO&& other) noexcept;

        void AddColorTexture(GLuint count, bool multisample = false);
        void SetColorTexture(GLenum index, GLuint cubemap_texture, GLuint face);
        void AddDepStTexture(bool multisample = false);
        void AddDepStRenderBuffer(bool multisample = false);

        const Texture& GetColorTexture(GLenum index) const;
        const Texture& GetDepthTexture() const;
        const TexView& GetStencilTexView() const;

        void Bind() const override;
        void Unbind() const override;
        void SetDrawBuffer(GLuint index) const;
        void SetDrawBuffers(std::vector<GLuint> indices) const;

        void Draw(GLint index) const;
        void Clear(GLint index) const;
        void ClearAll() const;

        static void TransferColor(const FBO& fr, GLuint fr_idx, const FBO& to, GLuint to_idx);
        static void TransferDepth(const FBO& fr, const FBO& to);
        static void TransferStencil(const FBO& fr, const FBO& to);
    };

}
