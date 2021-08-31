#pragma once

#include <string>
#include <GL/glew.h>

namespace components {

    class Texture {
      private:
        void SetTextures() const;
        void SetWrapMode() const;
        void SetFilterMode() const;

      public:
        GLuint id;
        GLenum target;     // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP
        std::string path;  // filepath of 1D/2D images, or folder that contains the cubemap faces

        Texture(GLenum target, const std::string& path);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;
    };

}
