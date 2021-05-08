#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <GL/glew.h>

#include "canvas.h"

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

    // forbid the copying of instances because they encapsulate global OpenGL resources and states.
    // when that happens, the old instance would probably be destructed and ruin the global states,
    // so we end up with a copy that points to an OpenGL object that has already been destroyed.
    // compiler-provided copy constructor and assignment operator only perform shallow copy.

    Texture(const Texture&) = delete;             // delete the copy constructor
    Texture& operator=(const Texture&) = delete;  // delete the assignment operator

    // it may be possible to override the copy constructor and assignment operator to make deep
    // copies and put certain constraints on the destructor, so as to keep the OpenGL states alive,
    // but that could be incredibly expensive or even impractical.

    // a better option is to use move semantics:
    Texture(Texture&& other) noexcept;             // move constructor
    Texture& operator=(Texture&& other) noexcept;  // move assignment operator
};
