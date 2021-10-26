#pragma once

#include <memory>
#include <vector>
#include "buffer/buffer.h"
#include "components/component.h"

// forward declaration
namespace components {
    class Shader;
    class Mesh;
    class Material;
}

namespace buffer {

    using namespace components;

    /* currently we do not support multisample FBOs, MSAA must be applied on the default framebuffer.
       
       for safety, by default we will disable the implicit colorspace correction (silently performed
       by the hardware), so that any fragment shader output will be written to the framebuffer AS IS.
       the fragment shader is free to decide what colorspace it wants to work in (which the user must
       be well aware of). For example, Gamma correction has to be done explicitly within the fragment
       shader, rather than relying on the hardware. In particular, if you need to work with blending
       (which is expected to interact with sRGB images) with framebuffers, make sure to linearize the
       sRGB color first, then do the blending in linear RGB space, and finally convert back to sRGB.

       note that it's safe to use pointers to a `Mesh` or `Material` here because framebuffers are not
       registered in the ECS pool, but in other classes, you should never rely on such a pointer since
       they are volatile (recall that the ECS pool is free to rearrange memory address as it sees fit)

       the virtual material is intended to be used by rendering operations in a later pass that depend
       on the textures data of an FBO. Every FBO has an empty virtual material by default on construct.
       for all kinds of postprocessing such as deferred lighting or shadow mapping or edge detection,
       users should first obtain a pointer to the virual material of the FBO that data was previously
       written to, then attach a custom shader by `SetShader()`, set custom uniforms by `SetUniform()`,
       and finally call `PostProcessDraw()` on the FBO, which automatically binds the virtual material,
       uploads textures to the GPU, draw the fullscreen quad (this step is skipped when the attached
       shader is a compute shader), and then unbind everything. Thus, users do not need to manually
       bind the textures of the FBO, it is done under the hood. But to write a shader that's compatible
       with the virtual material, make sure to use enough samplers to hold the textures data, and also
       pay attention to their orders: the list of `n` color attachments will be bound to texture units
       0 up to n-1 in sequence, depth and stencil component are then bound to texture unit n and n+1.

       keep in mind that `PostProcessDraw()` is used to postprocess the data in the FBO and then draws
       the result, it does not draw the FBO itself (the `DebugDraw()` function does that). How it draws
       and postprocesses the textures data is completely up to the user-provided shader, in all cases,
       it won't draw anything more than a fullscreen quad, so do not submit entities to the renderer
       when you are using this call. As long as the FBO is given a shader, it knows how to draw itself.

       user-defined FBOs are mostly used to hold temporary screen-space data in an intermediate pass
       where the main focus is to render intermediate results into the attached textures, such as the
       G-buffer used in deferred shading. Therefore, normally we don't want to draw them directly, but
       sometimes it can be helpful if we're able to visualize the content of such temporary buffer data
       for debugging purposes, thus we have the function `DebugDraw()`. This function uses a preset
       debug shader called "fullscreen.glsl", there's no need to write up your own. It takes as input
       a buffer id and draws the texture corresponding to that id. Here's an example:
           ......
           fbo.DebugDraw(i);   // draws one of the n color textures, where 0 <= i <= n-1
           fbo.DebugDraw(-1);  // visualize the linearized depth buffer
           fbo.DebugDraw(-2);  // visualize the stencil buffer
    */

    // forward declaration
    class RBO;
    class Texture;
    class TexView;

    class FBO : public Buffer {
      private:
        GLenum status;
        GLuint width, height;

        std::vector<Texture>     color_textures;      // the vector of color attachments
        std::unique_ptr<RBO>     depst_renderbuffer;  // depth & stencil as a renderbuffer
        std::unique_ptr<Texture> depst_texture;       // depth & stencil as a texture
        std::unique_ptr<TexView> stencil_view;        // temporary stencil texture view

        std::unique_ptr<Mesh>     virtual_mesh;       // for rendering the framebuffer (a quad)
        std::unique_ptr<Shader>   virtual_shader;     // for debugging the framebuffer
        std::unique_ptr<Material> virtual_material;   // for postprocessing the framebuffer

      public:
        FBO() = default;
        FBO(GLuint width, GLuint height);
        ~FBO();

        FBO(const FBO&) = delete;
        FBO& operator=(const FBO&) = delete;

        FBO(FBO&& other) noexcept;
        FBO& operator=(FBO&& other) noexcept;

        void AddColorTexture(GLuint count);
        void AddDepStTexture();
        void AddDepStRenderBuffer();

        const Texture& GetColorTexture(GLenum index) const;
        const Texture& GetDepthTexture() const;
        const TexView& GetStencilTexView() const;

        Material& GetVirtualMaterial();
        
        void Bind() const override;
        void Unbind() const override;

        void PostProcessDraw() const;
        void DebugDraw(GLint index) const;
        void Clear(GLint index) const;

        static void TransferColor(const FBO& fr, GLuint fr_idx, const FBO& to, GLuint to_idx);
        static void TransferDepth(const FBO& fr, const FBO& to);
        static void TransferStencil(const FBO& fr, const FBO& to);
    };

}
