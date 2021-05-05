#include "texture.h"

const std::unordered_map<GLenum, std::string> Texture::cubemap {
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X, "posx.png" },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "posy.png" },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "posz.png" },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "negx.png" },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "negy.png" },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "negz.png" }
};

void Texture::LoadTexture() const {
    stbi_set_flip_vertically_on_load(false);
    int width, height, n_channels;
    unsigned char* buffer = nullptr;

    if (target == GL_TEXTURE_2D) {
        buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);
        if (!buffer) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            stbi_image_free(buffer);
            return;
        }

        GLint format = (n_channels == 4 ? GL_RGBA : GL_RGB);
        glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);

        glGenerateMipmap(target);
        stbi_image_free(buffer);
    }
    else if (target == GL_TEXTURE_3D) {
        // solid textures, volume simulations, to be implemented...
    }
    else if (target == GL_TEXTURE_CUBE_MAP) {
        // skylight illumination, dynamic reflection, to be implemented...
    }
}

void Texture::LoadSkybox() const {
    stbi_set_flip_vertically_on_load(false);
    int width, height, n_channels;
    unsigned char* buffer = nullptr;

    for (const auto& face : cubemap) {
        std::string filepath = path + face.second;
        buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

        if (!buffer) {
            std::cerr << "Failed to load skybox texture: " << filepath << std::endl;
            stbi_image_free(buffer);
            return;
        }
        else {
            glTexImage2D(face.first, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        }

        stbi_image_free(buffer);
    }
}

void Texture::SetWrapMode() const {
    if (target == GL_TEXTURE_2D) {
        // repeat the texture image (recommend to use seamless textures)
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else {
        // repeat the last pixels when s/t/r coordinates fall off the edge
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
}

void Texture::SetFilterMode(bool anisotropic) const {
    // -------------------------------------------------------------------------------------------
    // [1] -> [4]: (from cheapest to most expensive, from worst to best visual quality)
    // -------------------------------------------------------------------------------------------
    // [1] point filtering produces a blocked pattern (individual pixels more visible)
    // [2] bilinear filtering produces a smooth pattern (texel colors are sampled from neighbors)
    // [3] trilinear filtering linearly interpolates between two bilinearly sampled mipmaps
    // [4] anisotropic filtering samples the texture as a non-square shape to correct blurriness
    // -------------------------------------------------------------------------------------------
    if (target == GL_TEXTURE_2D) {
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // trilinear filtering

        // anisotropic filtering requires OpenGL core 4.6 or EXT_texture_filter_anisotropic
        if (anisotropic) {
            GLfloat param = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &param);

            param = std::clamp(param, 1.0f, 8.0f);  // implementation-defined maximum anisotropy
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, param);
        }
    }
    else if (target == GL_TEXTURE_3D) {
        // solid textures, volume simulations, to be determined...
    }
    else if (!type.compare("skybox")) {
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // skyboxes do not minify, no mipmaps
    }
    else if (target == GL_TEXTURE_CUBE_MAP) {
        // skylight illumination, dynamic reflection, to be determined...
    }
}

Texture::Texture(GLenum target, const std::string& type, const std::string& path, bool anisotropic)
    : target(target), type(type), path(path) {

    glGenTextures(1, &id);
    glBindTexture(target, id);

    if (target == GL_TEXTURE_CUBE_MAP && !type.compare("skybox")) {
        LoadSkybox();
    }
    else {
        LoadTexture();
    }

    SetWrapMode();
    SetFilterMode(anisotropic);

    glBindTexture(target, 0);
}

Texture::~Texture() {
    glBindTexture(target, 0);
    glDeleteTextures(1, &id);
}
