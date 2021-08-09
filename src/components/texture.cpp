#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "components/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace core;

namespace components {

    void Texture::SetTextures() const {
        static const std::vector<std::string> extensions { ".jpg", ".png", ".jpeg" };
        static const std::unordered_map<GLenum, std::string> cubemap {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, "posx" },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "posy" },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "posz" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "negx" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "negy" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "negz" }
        };

        stbi_set_flip_vertically_on_load(false);
        int width, height, n_channels;
        uint8_t* buffer = nullptr;

        if (target == GL_TEXTURE_2D) {
            CORE_INFO("Loading textures from: {0}", path);
            buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);

            if (!buffer) {  // buffer == nullptr
                CORE_ERROR("Failed to load texture: {0}", path);
                CORE_ERROR("STBI failure reason: {0}", stbi_failure_reason());
                return;
            }

            switch (n_channels) {
                case 3:
                    glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
                    break;
                case 4:
                    glTexImage2D(target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
                    break;
                default:
                    CORE_ERROR("Non-standard image format with {0} channels: {1}", n_channels, path);
                    break;
            }

            glGenerateMipmap(target);
            stbi_image_free(buffer);
        }

        // solid textures, volume simulations ...
        else if (target == GL_TEXTURE_3D) {

        }

        // skybox, skylight illumination, dynamic reflection ...
        else if (target == GL_TEXTURE_CUBE_MAP) {
            CORE_INFO("Loading cubemaps from: {0}", path);

            // infer image file extension
            std::string extension = "";

            for (auto& ext : extensions) {
                std::string filepath = path + "posx" + ext;
                buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

                if (buffer) {
                    extension = ext;
                    break;
                }
            }

            CORE_ASERT(!extension.empty(), "Cannot find textures in {0}: bad path or file extension)", path);

            for (const auto& face : cubemap) {
                std::string filepath = path + face.second + extension;
                buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

                if (!buffer) {
                    CORE_ERROR("Failed to load texture: {0}", filepath);
                    CORE_ERROR("STBI failure reason: {0}", stbi_failure_reason());
                    return;
                }

                if (!(n_channels == 3 || n_channels == 4)) {
                    CORE_ERROR("Invalid number of channels: {0}", filepath);
                }

                glTexImage2D(face.first, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
                stbi_image_free(buffer);
            }
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

    // [side note on filtering modes] cheapest -> most expensive, worst -> best visual quality
    // -----------------------------------------------------------------------------------------
    // [1] point filtering produces a blocked pattern (individual pixels more visible)
    // [2] bilinear filtering produces a smooth pattern (texel colors are sampled from neighbors)
    // [3] trilinear filtering linearly interpolates between two bilinearly sampled mipmaps
    // [4] anisotropic filtering samples the texture as a non-square shape to correct blurriness

    void Texture::SetFilterMode() const {
        if (target == GL_TEXTURE_2D) {
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // trilinear filtering

            // anisotropic filtering requires OpenGL core 4.6 or EXT_texture_filter_anisotropic
            GLfloat param = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &param);

            param = std::clamp(param, 1.0f, 8.0f);  // implementation-defined maximum anisotropy
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, param);
        }

        // solid textures, volume simulations, to be determined...
        else if (target == GL_TEXTURE_3D) {

        }

        // skybox, skylight illumination, dynamic reflection, to be determined...
        else if (target == GL_TEXTURE_CUBE_MAP) {
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // skyboxes do not minify, no mipmaps
        }
    }

    Texture::Texture(GLenum target, const std::string& path) : target(target), path(path) {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        glGenTextures(1, &id);
        glBindTexture(target, id);

        SetTextures();
        SetWrapMode();
        SetFilterMode();

        glBindTexture(target, 0);
    }

    Texture::~Texture() {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // log message to the console so that we are aware of the *hidden* destructor calls
        // this can be super useful in case our data accidentally goes out of scope
        if (id > 0) {
            CORE_WARN("Destructing texture data (target = {0}, id = {1})!", target, id);
        }

        glDeleteTextures(1, &id);
    }

    Texture::Texture(Texture&& other) noexcept {
        *this = std::move(other);
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this != &other) {
            glDeleteTextures(1, &id);
            id = 0;
            path = "";

            std::swap(id, other.id);
            std::swap(target, other.target);
            std::swap(path, other.path);
        }

        return *this;

        // after the swap, the other texture has an id of 0, which is a clean null state
        // (a default fallback texture that is all black), when it gets destroyed later,
        // deleting a 0-id texture won't break the global binding states so we are safe.
    }
}
