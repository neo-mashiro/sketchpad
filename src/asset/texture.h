/* 
   for this demo we are going to use immutable storage textures as much as possible since we
   are targeting at OpenGL 4.6 and above. Once the texture is allocated, immutable storage
   cannot change its size, format, and number of layers, which are fixed in GPU memory, but
   the texture content itself is still modifiable. The purpose of using immutable storage is
   to avoid runtime consistency checks and ensure type safety, so that rendering operations
   are able to run faster.

   this class provides a number of ways to build textures of various types, users can either
   load image files from local disk, load faces from a directory, or create an empty texture
   for use as a render target in a shader or framebuffer, see examples below.

   the `levels` parameter refers to the level of detail (LOD) number, which is the number of
   mipmap levels in the texture. A value of 1 indicates no mipmaps, so that the texture only
   has a base layer. If it's 0, the number of mip levels is deduced from the texture's width
   height and depth, and the chain of mipmaps will be generated automatically.

   # example

   - Texture("../albedo.png", 0);                // load a regular image into a 2D texture, with mipmaps
   - Texture("../screen.png", 1);                // load a regular image into a 2D texture, base layer only
   - Texture("../equirectangular.hdr", 1);       // load a panorama HDRI into a 2D texture, no mipmaps
   - Texture("../equirectangular.hdr", 512, 1);  // load a panorama HDRI into a cubemap texture, no mipmaps
   - Texture("../equirectangular.jpg", 512, 1);  // load a regular image into a cubemap texture, low quality
   - Texture("../cubemap", ".hdr", 512, 1);      // load 6 separate faces into a cubemap texture, no mipmaps

   - Texture(GL_TEXTURE_2D, 256, 256, 1, GL_RG16F, 1);          // make an empty 2D BRDF LUT, no mipmaps
   - Texture(GL_TEXTURE_2D, 256, 256, 1, GL_RGB16F, 1);         // make an empty 3D BRDF LUT, for cloth
   - Texture(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);  // make an empty irradiance map, no mipmaps
   - Texture(GL_TEXTURE_CUBE_MAP, 512, 512, 6, GL_RGBA16F, 0);  // make an empty environment map, with mipmaps

   # smart bindings

   just like the shader and vao class, this class will keep track of texture in each texture
   unit to avoid unnecessary bindings, trying to bind a texture that's already bound to the
   given unit has 0 overhead, there will be no context switching cost in this case. However,
   this only applies to textures and texture views, image load store (ILS) is not included.
   there's also a new feature called "bindless textures", which is not in the core profile
   yet and still lacks support on many GPU drivers so I'll ignore it for now. Note that the
   depth of a texture specifies the number of layers, for 2D textures, it must be 1, not 0.

   # texture types

   for most use cases we are going to work with 2D textures and cubemaps, or arrays of them.
   other types are rarely used, and should generally be avoided. For example, the high cost
   of switching textures resulted in a widespread preference for texture atlases, which is a
   single texture that packs together many images to reduce the context switching overhead,
   atlases can be created using 3D textures, but 3D texture is often a very poor substitute,
   instead, textures array would be a much better alternative. 1D texture is also a relic of
   the old days, it's commonly used as a bus to send data to GLSL so isn't really a texture,
   there's also sth called buffer textures, which are backed by buffer objects and hence the
   name, but since we now have SSBOs, 1D textures and buffer textures are just useless.

   # texture views

   data within a texture can be shared through multiple texture views, possibly in different
   formats. An example is the framebuffer texture that stores both depth and stencil values
   in the `GL_DEPTH24_STENCIL8` format, in this case we can access the stencil buffer via a
   view of that texture.

   the `TexView` class is using a naive dependency injection, the host texture is not owned
   by the view and thus its lifetime is not bound to the view either. To be clear, views do
   not keep their host textures alive, the host lives and dies on its own. Since a view can
   not exist on its own, we have to make sure that hosts always outlive their views, o/w we
   would have a view of nothing and possible undefined behavior.

   # image load store (ILS)

   despite the name ILS, image load store is just an "image", a single level of image from a
   texture. Image here refers to a layer of the texture, or a specific mipmap level. While
   textures can have multiple mipmap levels or array layers, an image can only represent one
   of them. Unlike texture views, an image is just a reference to a given layer of the host
   texture, it doens't hold any data. It also cannot exist on its own (like texture views).
   as soon as the texture is destroyed, the ILS becomes invalid, we don't need a destructor.

   images have their own set of binding points called image units, which are independent of
   texture units and are counted separately. Images are essentially a large 2D array, so the
   pixels can only be accessed by (signed) integer indices. Note that floating-point indices
   cannot be used to access an ILS, which also means that no filtering will be applied, that
   images do not work with samplers. For image load store operations, reading from any texel
   outside the boundaries will return 0 and writing to any texel outside the boundaries will
   do nothing, so we can safely ignore all boundary checks.

   ILS is often used in pair with a compute shader, where it plays the role of data buffers.
   an image can be bound to multiple image units at the same time, in this case, coherency
   of memory accesses must be taken care of. If it's only bound to one image unit, we should
   generally use the `restrict` memory qualifier so that reads/writes can be optimized.

   # texture vs ILS (in shader)

   the advantage of using image load store over regular textures (whether storage is mutable
   or not) lies in its flexibility in terms of both read and write operations from within
   the shader. These operations are cheap, mostly atomic, but you need to manage incoherent
   memory access with proper barrier calls. Equipped with the powerful features of ILS, we
   can manipulate the data store in a number of new ways. In some cases, using ILS can save
   us from creating an extra FBO, but it cannot handle blending or depth stencil testing.

   ILS can be used to implement relatively cheap order-independent transparency (OIT).
   ILS is the best tool to implement temporal anti-aliasing (TAA), both the past and current
   frame can be represented by ILS so that sampling and blending pixels are made much easier.
*/

#pragma once

#include <string>
#include <glad/glad.h>
#include "asset/asset.h"

namespace asset {

    class Texture;  // forward declaration

    class TexView : public IAsset {
      public:
        const Texture& host;
        TexView(const Texture& texture);
        TexView(const Texture&& texture) = delete;  // prevent rvalue references to temporary objects
        ~TexView();

        TexView(const TexView&) = delete;
        TexView& operator=(const TexView&) = delete;
        TexView(TexView&& other) = default;
        TexView& operator=(TexView&& other) = default;

        void SetView(GLenum target, GLuint fr_level, GLuint levels, GLuint fr_layer, GLuint layers) const;
        void Bind(GLuint index) const;
        void Unbind(GLuint index) const;
    };

    class Texture : public IAsset {
      private:
        friend class TexView;
        GLenum target;
        GLenum format, i_format;  // internal format
        void SetSampleState() const;

      public:
        GLuint width, height, depth;
        GLuint n_levels;

        Texture(const std::string& img_path, GLuint levels = 0);
        Texture(const std::string& img_path, GLuint resolution, GLuint levels);
        Texture(const std::string& directory, const std::string& extension, GLuint resolution, GLuint levels);
        Texture(GLenum target, GLuint width, GLuint height, GLuint depth, GLenum i_format, GLuint levels);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept = default;
        Texture& operator=(Texture&& other) noexcept = default;

        void Bind(GLuint index) const override;
        void Unbind(GLuint index) const override;
        void BindILS(GLuint level, GLuint index, GLenum access) const;
        void UnbindILS(GLuint index) const;

        void GenerateMipmap() const;
        void Clear(GLuint level) const;
        void Invalidate(GLuint level) const;

        static void Copy(const Texture& fr, GLuint fr_level, const Texture& to, GLuint to_level);
    };

}