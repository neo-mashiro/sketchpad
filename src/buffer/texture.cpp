#include "pch.h"

#include "core/log.h"
#include "utils/image.h"
#include "buffer/texture.h"

namespace buffer {

    void Texture::SetWrapMode() const {
        if (target == GL_TEXTURE_2D) {
            // 2D textures are mostly seamless, simply repeat the texture
            glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else if (target == GL_TEXTURE_CUBE_MAP) {
            // cubemaps must be clamped on the edges, i.e. repeat the last pixels when
            // s/t/r coordinates fall off the edges. This is to prevent edge sampling
            // artifacts in environment maps, irradiance maps and BRDF lookup textures.
            glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
        else {
            throw core::NotImplementedError("texture type not yet implemented...");
        }
    }

    void Texture::SetFilterMode() const {
        // for texture magnification, bilinear filtering is more than enough
        // for texture minification, trilinear filtering is necessary if we were to sample mipmaps
        GLint mag_filter = GL_LINEAR;
        GLint min_filter = n_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

        // anisotropic filtering requires OpenGL 4.6, where maximum anisotropy is implementation-defined
        GLfloat max_anisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
        GLfloat anisotropy = std::clamp(max_anisotropy, 1.0f, 8.0f);  // limit anisotropy to 8

        if (target == GL_TEXTURE_2D) {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
            glTextureParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
        }
        else if (target == GL_TEXTURE_CUBE_MAP) {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
        }
        else {
            throw core::NotImplementedError("texture type not yet implemented...");
        }
    }

    Texture::Texture(const std::string& path, GLsizei levels)
        : Buffer(), width(0), height(0), n_levels(levels), format(0), internal_format(0) {

        target = GL_TEXTURE_2D;
        auto image = utils::Image(path);

        width = image.GetWidth();
        height = image.GetHeight();
        format = image.GetFormat();
        internal_format = image.GetInternalFormat();
        
        // if levels is 0, automatic compute the number of mipmap levels needed
        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(std::floor(std::log2(std::max(width, height))));
        }

        glCreateTextures(target, 1, &id);
        glTextureStorage2D(id, n_levels, internal_format, width, height);

        if (image.IsHDR()) {
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_FLOAT, image.GetPixels<float>());
        }
        else {
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, image.GetPixels<uint8_t>());
        }

        if (n_levels > 1) {
            glGenerateTextureMipmap(id);
        }

        SetWrapMode();
        SetFilterMode();
    }

    Texture::Texture(GLenum target, GLuint width, GLuint height, GLenum internal_format, GLuint levels)
        : Buffer(), target(target), width(width), height(height),
          n_levels(levels), format(0), internal_format(internal_format) {

        // if levels is 0, automatic compute the number of mipmap levels needed
        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(std::floor(std::log2(std::max(width, height))));
        }

        glCreateTextures(target, 1, &id);
        glTextureStorage2D(id, n_levels, internal_format, width, height);

        SetWrapMode();
        SetFilterMode();
    }

    Texture::~Texture() {
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
            // it's safe to delete texture 0 (a default fallback texture that is all black)
            this->id = 0;

            std::swap(id, other.id);
            std::swap(target, other.target);
            std::swap(width, other.width);
            std::swap(height, other.height);
            std::swap(n_levels, other.n_levels);
            std::swap(format, other.format);
            std::swap(internal_format, other.internal_format);
        }

        return *this;
    }

    void Texture::Bind(GLuint unit) const {
        // the DSA call does not alter the global state of active texture unit
        glBindTextureUnit(unit, id);
    }

    void Texture::Unbind(GLuint unit) const {
        glBindTextureUnit(unit, 0);
    }

    void Texture::Clear() const {
        if (internal_format == GL_RGBA16F) {
            std::vector<GLubyte> zeros(width * height, 0);
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_FLOAT, &zeros[0]);
        }
        else {
            std::vector<GLubyte> zeros(width * height, 0);
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, &zeros[0]);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    TexView::TexView(const Texture& texture, GLuint levels) : Buffer() {
        glGenTextures(1, &id);
        glTextureView(id, texture.target, texture.id, texture.internal_format, 0, levels, 0, 1);
    }

    TexView::~TexView() {
        glDeleteTextures(1, &id);
    }

    void TexView::Bind(GLuint unit) const {
        glBindTextureUnit(unit, id);
    }

    void TexView::Unbind(GLuint unit) const {
        glBindTextureUnit(unit, 0);
    }

}
