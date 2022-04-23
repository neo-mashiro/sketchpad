/* 
   for safety, by default we will disable the implicit colorspace correction (silently performed
   by the hardware), so that any fragment shader output will be written to the framebuffer AS IS.
   the fragment shader is free to decide what colorspace it wants to work in, which the user must
   be well aware of. In specific, gamma correction has to be done explicitly within the fragment
   shader, rather than relying on the hardware. In particular, if you need to work with blending
   (which is expected to interact with sRGB images) with framebuffers, make sure to linearize the
   sRGB color first, then do the blending in linear RGB space, and finally convert back to sRGB.

   # debug draw a single buffer

   user-defined FBOs are mostly used to hold temporary screen-space data in an intermediate pass
   where the main focus is to render intermediate results into the attached textures. Normally we
   don't want to draw them directly, but sometimes we may want to visualize the contents of these
   temporary buffer for debugging purposes. The `Draw()` function does just that, which uses a
   preset debug shader (bufferless rendering).
*/

#pragma once

#include <vector>
#include "core/base.h"
#include "asset/texture.h"

namespace asset {

    class RBO : public IAsset {
      public:
        RBO(GLuint width, GLuint height, bool multisample = false);
        ~RBO();

        void Bind() const override;
        void Unbind() const override;
    };

    class FBO : public IAsset {
      private:
        GLenum status;
        GLuint width, height;

        std::vector<Texture> color_attachments;   // vector of color attachments
        asset_tmp<RBO>       depst_renderbuffer;  // depth and stencil as a single renderbuffer
        asset_tmp<Texture>   depst_texture;       // depth and stencil as a single texture
        asset_tmp<TexView>   stencil_view;        // stencil as a temporary texture view

      public:
        FBO() = default;
        FBO(GLuint width, GLuint height);
        ~FBO();

        FBO(const FBO&) = delete;
        FBO& operator=(const FBO&) = delete;
        FBO(FBO&& other) noexcept = default;
        FBO& operator=(FBO&& other) noexcept = default;

      public:
        void AddColorTexture(GLuint count, bool multisample = false);
        void SetColorTexture(GLenum index, GLuint texture_2d);
        void SetColorTexture(GLenum index, GLuint texture_cubemap, GLuint face);
        void AddDepStTexture(bool multisample = false);
        void AddDepStRenderBuffer(bool multisample = false);
        void AddDepthCubemap();

        const Texture& GetColorTexture(GLenum index) const;
        const Texture& GetDepthTexture() const;
        const TexView& GetStencilTexView() const;

        void Bind() const override;
        void Unbind() const override;

        void SetDrawBuffer(GLuint index) const;
        void SetDrawBuffers(std::vector<GLuint> indices) const;
        void SetDrawBuffers() const;

        void Draw(GLint index) const;
        void Clear(GLint index) const;
        void Clear() const;

      public:
        static void CopyColor(const FBO& fr, GLuint fr_idx, const FBO& to, GLuint to_idx);
        static void CopyDepth(const FBO& fr, const FBO& to);
        static void CopyStencil(const FBO& fr, const FBO& to);
    };

}