#pragma once

#include <string>
#include <GL/glew.h>
#include "buffer/buffer.h"

namespace buffer {

    /* since our demo is targeted at OpenGL 4.6 and above, we'll use immutable storage textures
       exclusively, whose storage cannot be changed once the texture is allocated. That is, the
       size, format, and number of layers are fixed in memory, but the texture content itself is
       still modifiable. The purpose of using immutable storage is to avoid runtime consistency
       checks and ensure type safety, so that rendering operations are able to run faster.

       the first ctor creates a texture by loading an image file from the given path, the pixels
       data of the image is immediately copied into the texture. The other ctor is used to create
       an empty texture of the specified target, width, height and internal format, so that users
       can write to it at a later time, this is often used to hold pre-computed data before the
       rendering loop, such as pre-filtered environment maps, irradiance maps and the BRDF lookup
       textures. Such user-defined textures are better-suited for this task than the ILS buffer
       because they can be directly attached to user-defined framebuffers.

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

       > Texture(GL_TEXTURE_2D, 256, 256, GL_RG16F, 1);         // create an empty BRDF lookup texture, no mipmaps
       > Texture(GL_TEXTURE_CUBE_MAP, 32, 32, GL_RGB16F, 1);    // create an empty irradiance map, no mipmaps
       > Texture(GL_TEXTURE_CUBE_MAP, 512, 512, GL_RGB16F, 0);  // create an empty environment map, with mipmaps

       # smart bindings

       just like the shader and uniform class, this class keeps track of textures in each texture
       unit to avoid unnecessary binding operations, trying to bind a texture that is already in
       the given texture unit has 0 overhead, there's no context switching in this case. However,
       this feature only applies to textures and texture views, excluding image load/store (ILS).
    */

    class Texture : public Buffer {
      private:
        friend class Sampler;
        friend class TexView;

        GLenum target;
        GLenum format, internal_format;
        GLuint n_levels;

        void Bind() const override {}
        void Unbind() const override {}

      public:
        GLuint width, height;

        Texture(const std::string& img_path, GLuint levels = 0);
        Texture(const std::string& img_path, GLuint resolution, GLuint levels);
        Texture(const std::string& directory, const std::string& extension, GLuint resolution, GLuint levels);
        Texture(GLenum target, GLuint width, GLuint height, GLenum internal_format, GLuint levels, bool multisample = false);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        void Bind(GLuint unit) const;
        void Unbind(GLuint unit) const;

        void Clear() const;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////

    class TexView : public Buffer {
      private:
        void Bind() const override {}
        void Unbind() const override {}

      public:
        TexView(const Texture& texture, GLuint levels = 1);
        ~TexView();

        void Bind(GLuint unit) const;
        void Unbind(GLuint unit) const;
    };

}
