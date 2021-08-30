#pragma once

#include <string>
#include <GL/glew.h>

namespace components {

    enum class TextureType : uint8_t {
        None             = 0,
        Ambient          = 1,
        Diffuse          = 2,
        Specular         = 3,
        Emissive         = 4,
        Shininess        = 5,
        Normal           = 6,
        Height           = 7,
        Opacity          = 8,
        Displacement     = 9,
        Lightmap         = 10,  // a.k.a ambient occlusion
        BaseColor        = 11,  // PBR standard
        NormalCamera     = 12,  // PBR standard
        EmissionColor    = 13,  // PBR standard
        Metalness        = 14,  // PBR standard
        DiffuseRoughness = 15,  // PBR standard
        AmbientOcclusion = 16   // PBR standard
    };

    class Texture {
      private:
        void SetTextures() const;
        void SetWrapMode() const;
        void SetFilterMode() const;

      public:
        GLuint id;
        GLenum target;     // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP
        std::string path;  // filepath of 1D/2D images, or folder that contains the cubemap faces
        TextureType type;

        Texture(GLenum target, const std::string& path);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;
    };

}
