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

         Texture(GL_TEXTURE_2D, 256, 256, GL_RG16F);               // BRDF integration map (single layer)
         Texture(GL_TEXTURE_CUBE_MAP, 32, 32, GL_RGBA16F);         // irradiance map (single layer)
         Texture(GL_TEXTURE_CUBE_MAP, 1024, 1024, GL_RGBA16F, 0);  // environment map (with mipmaps)

       # texture views: https://www.khronos.org/opengl/wiki/Texture_Storage#Texture_views
    */

    class TexView;

    class Texture : public Buffer {
      private:
        friend class TexView;
        GLenum target;
        GLenum format, internal_format;
        GLuint width, height, n_levels;

        void Bind() const override {}
        void Unbind() const override {}

        void SetWrapMode() const;
        void SetFilterMode() const;

      public:
        Texture(const std::string& path, GLsizei levels = 0);
        Texture(GLenum target, GLuint width, GLuint height, GLenum internal_format, GLuint levels = 1);
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
