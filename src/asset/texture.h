#pragma once

#include <string>
#include <glad/glad.h>
#include "asset/asset.h"

namespace asset {

    /* since our demo is targeted at OpenGL 4.6 and above, we'll use immutable storage textures
       exclusively, whose storage cannot be changed once the texture is allocated. That is, the
       size, format, and number of layers are fixed in memory, but the texture content itself is
       still modifiable. The purpose of using immutable storage is to avoid runtime consistency
       checks and ensure type safety, so that rendering operations are able to run faster.

       this class provides a number of ctors for building textures in different scenarios, users
       can create textures by loading from an image file, or from multiple files in a directory.
       there's also a variant that allows you to build a custom empty texture, which can be used
       as a render target in a shader or framebuffer, see examples below.

       the `levels` parameter in the ctor refers to the level-of-detail (LOD) number, which is
       the number of mipmap levels of the texture. A value of 0 indicates no mipmaps, so that the
       texture only has a single base layer. If levels is 0, the ctor will automatically figure
       out the number of mipmap levels from texture's width and height and generate all mipmaps.

       # examples

       > Texture("../albedo.png", 0);                // load a normal image into a 2D texture, with mipmaps
       > Texture("../screen.png", 1);                // load a normal image into a 2D texture, base layer only
       > Texture("../equirectangular.hdr", 1);       // load an HDR image as a 2D texture, no mipmaps
       > Texture("../equirectangular.hdr", 512, 1);  // load an HDR image as a cubemap texture, no mipmaps
       > Texture("../equirectangular.jpg", 512, 1);  // load a normal image as a cubemap texture, low quality

       > Texture("../cubemap", ".exr", 512, 1);      // load 6 separate faces into a cubemap texture, no mipmaps
       > Texture("../cubemap", ".hdr", 1024, 0);     // this ctor only accepts ".hdr" and ".exr" file extensions

       > Texture(GL_TEXTURE_2D, 256, 256, 1, GL_RG16F, 1);         // create an empty BRDF lookup texture, no mipmaps
       > Texture(GL_TEXTURE_CUBE_MAP, 32, 32, 6, GL_RGB16F, 1);    // create an empty irradiance map, no mipmaps
       > Texture(GL_TEXTURE_CUBE_MAP, 512, 512, 6, GL_RGB16F, 0);  // create an empty environment map, with mipmaps

       # smart bindings

       just like the shader and uniform class, this class keeps track of textures in each texture
       unit to avoid unnecessary binding operations, trying to bind a texture that is already in
       the given texture unit has 0 overhead, there's no context switching in this case. However,
       this feature only applies to textures and texture views, excluding image load/store (ILS).

       # bindless textures

       this feature hasn't yet been included into the core profile, it still has several concerns
       and is not supported on many Intel cards, so I will leave this to further releases.
    */

    /* despite the name ILS, image load store is just an "image", a single level of image from a
       texture. Image here refers to a layer in the texture, not the image object that is loaded
       from local image files. While textures can have multiple mipmap levels or array layers,
       images can only represent one of them. An image is merely a layered reference to the host
       texture, which does not hold extra data and cannot exist by itself (like texture views).
       as soon as the texture is destroyed, the ILS becomes invalid, we don't need a destructor.

       images have their own set of binding points called image units, which are independent of
       texture units and are counted separately. Images are essentially a large 2D array, so the
       pixels can only be accessed by (signed) integer indices, as such, floating-point indices
       are not allowed, and you must make sure no index is out of bound. This also means that no
       filtering will be applied, as a result, images do not work with samplers.

       for image load store, reads and writes are accessed via a list of GLSL built-in functions.
       ILS is often used in pair with a compute shader, the images play the role of data buffers.
       an image can be bound to multiple image units at the same time, in this case, coherency
       of memory accesses must be taken care of. If it's only bound to one image unit, we should
       always use the `restrict` memory qualifier so that reads/writes can be optimized.
       see also: https://www.khronos.org/opengl/wiki/Image_Load_Store#Image_operations

       # texture in shader vs ILS in shader

       the advantage of using image load/store over regular textures (whether storage is mutable
       or not) lies in its flexibility in terms of both read and write operations from within
       the shader. These operations are cheap, mostly atomic, but you need to manage incoherent
       memory access with proper barrier calls. Equipped with the powerful features of ILS, the
       user is able to manipulate the data store in a number of new ways.

       aside from textures, it bares emphasizing that ILS is not the same as FBO or PBO. In some
       cases, using ILS can save us from creating an extra FBO, but it cannot handle blending or
       depth stencil testing. Also, ILS is only concerned with read/write operations on textures,
       it has nothing to do with transfer operations, do not confuse it with PBO or TFB.

       # example use cases

       ILS can be used to implement relatively cheap order-independent transparency (OIT).
       ILS is the best tool to implement temporal anti-aliasing (TAA), both the past and current
       frame can be represented by ILS so that sampling and blending pixels are made much easier.
    */

    // 3D textures are a very poor substitute for texture atlases
    // The high cost of texture switches resulted in a widespread preference for texture atlases (a single texture which stores data for multiple objects). This resulted in fewer texture switching, and thus less overhead. Array textures are an alternative to atlases
    // the depth of the texture specifies the number of layers. for 2D textures, it must be 1, not 0!

    // 1D textures is also a relic of the old days, it is used to provide GLSL-accessible data, so not really a "texture", and
    // in this regard, one might be better off with buffer textures, which are safer as they are backed by other buffer objects
    // like a VBO.
    // nowadays we have SSBO, buffer textures become useless, so does 1D textures

    // naive dependency injection
    // the host texture is not owned by the view and thus its lifetime is not bound to the view
    // in other words, views do not keep their host textures alive, the hosts live/die on their own
    // the host texture's lifetime must outlive the view, otherwise we have undefined behavior

    class Texture;  // forward declaration

    class TexView : public IAsset {
      public:
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

      private:
        const Texture& host;
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