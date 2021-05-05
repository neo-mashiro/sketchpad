#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Texture {
  private:
    const static std::unordered_map<GLenum, std::string> cubemap;

    void LoadTexture() const;
    void LoadSkybox() const;
    void SetWrapMode() const;
    void SetFilterMode(bool anisotropic) const;

  public:
    GLuint id;
    GLenum target;     // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP
    std::string type;
    std::string path;  // image file path (1D/2D), or directory that contains images of 6 faces (3D/cubemap)

    // --------------------------------------------------------------------------------------------
    // supported texture types: must match the sampler names in GLSL
    // --------------------------------------------------------------------------------------------
    // ambient, diffuse, specular, emission, opacity
    // albedo, normal, bump/height, displacement, metallic, gloss/roughness
    // skybox (3D), skylight illumination (3D) ...... ssao, hdr, pbr ......
    // --------------------------------------------------------------------------------------------

    Texture(GLenum target, const std::string& type, const std::string& path, bool anisotropic = false);
    ~Texture();
};
