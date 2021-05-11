#include "texture.h"
#include "log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Sketchpad {

    const std::unordered_map<GLenum, std::string> Texture::cubemap {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, "posx" },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "posy" },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "posz" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "negx" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "negy" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "negz" }
    };

    void Texture::LoadTexture() const {
        stbi_set_flip_vertically_on_load(false);
        int width, height, n_channels;
        unsigned char* buffer = nullptr;

        if (target == GL_TEXTURE_2D) {
            buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);

            if (!buffer) {
                CORE_ERROR("Failed to load texture: {0}", path);
                CORE_ERROR("stbi failure reason: {0}", stbi_failure_reason());
                stbi_image_free(buffer);
                return;
            }

            GLint format = 0;
            switch (n_channels) {
                case 3: format = GL_RGB; break;
                case 4: format = GL_RGBA; break;
                default:
                    CORE_WARN("Non-standard image format: {0}", path);
                    CORE_WARN("This texture image has {0} channels", n_channels);
                    break;
            }

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
        std::string extension = ".jpg";

        try {
            std::string filepath = path + "posx" + extension;
            if (!stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha)) {
                throw "Non-JPG image format detected";
            }
        }
        catch (const char* error_extension) {
            CORE_INFO("{0}, inferring file extension...", error_extension);
            extension = ".png";
        }

        for (const auto& face : cubemap) {
            std::string filepath = path + face.second + extension;
            buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

            if (!buffer) {
                CORE_ERROR("Failed to load skybox: {0}", filepath);
                CORE_ERROR("stbi failure reason: {0}", stbi_failure_reason());
                stbi_image_free(buffer);
                return;
            }
            else {
                assert(n_channels == 3 || n_channels == 4);
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

        Canvas::CheckOpenGLContext("Texture");

        glGenTextures(1, &id);
        glBindTexture(target, id);

        if (target == GL_TEXTURE_CUBE_MAP && !type.compare("skybox")) {
            CORE_INFO("Loading cubemaps in folder: {0}", path);
            LoadSkybox();
        }
        else {
            CORE_INFO("Loading texture file: {0}", path);
            LoadTexture();
        }

        SetWrapMode();
        SetFilterMode(anisotropic);

        glBindTexture(target, 0);
    }

    Texture::~Texture() {
        // keep in mind that most OpenGL calls have global states, which in some cases might conflict
        // with the object oriented approach in C++, because class instances have their own scope.
        // chances are you don't want this to be called, unless you have removed the mesh from the scene.

        Canvas::CheckOpenGLContext("~Texture");

        // log friendly message to the console, so that we are aware of the *hidden* destructor calls
        // this is super useful in case our data accidentally goes out of scope, debugging made easier!
        if (id > 0) {  // texture may be still in use
            CORE_WARN("Destructing texture data (target = {0}, id = {1})!", target, id);
        }

        glBindTexture(target, 0);
        glDeleteTextures(1, &id);
    }

    Texture::Texture(Texture&& other) noexcept {
        *this = std::move(other);
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this != &other) {
            // free this texture, reset it to a clean null state
            glDeleteTextures(1, &id);
            id = 0;
            type = path = "";
            target = other.target;  // force target to be a valid GLenum so the state is clean

            // transfer ownership from other to this
            std::swap(id, other.id);
            std::swap(target, other.target);
            std::swap(type, other.type);
            std::swap(path, other.path);
        }

        // after the swap, the other texture has an id of 0, which is a clean null state
        // (a default fallback texture that is all black), when it gets destroyed later,
        // deleting a 0-id texture won't break the global binding states so we are safe.
        return *this;
    }
}
