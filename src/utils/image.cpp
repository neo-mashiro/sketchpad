#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "core/log.h"
#include "utils/image.h"

namespace utils {

    void Image::deleter::operator()(uint8_t* buffer) {
        stbi_image_free(buffer);
    }

    Image::Image(const std::string& filepath, GLuint read_channels, bool flip) : width(0), height(0), n_channels(0) {
        stbi_set_flip_vertically_on_load(flip);

        // supported file extensions (will support ".psd", ".tga" and ".gif" in the future)
        static const std::vector<std::string> extensions {
            ".jpg", ".png", ".jpeg", ".bmp", ".hdr"
        };

        // check valid file extension
        auto ext = filepath.substr(filepath.rfind("."));
        if (std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
            CORE_ERROR("Image file format is not supported: {0}", filepath);
            return;
        }

        CORE_INFO("Loading image from: {0}", filepath);
        this->is_hdr = stbi_is_hdr(filepath.c_str());

        if (is_hdr) {
            float* buffer = stbi_loadf(filepath.c_str(), &width, &height, &n_channels, 3);

            if (buffer == nullptr) {
                CORE_ERROR("Failed to load image: {0}", filepath);
                CORE_ERROR("STBI failure reason: {0}", stbi_failure_reason());
                return;
            }

            // report HDR image statistics before the buffer pointer is reset (useful in shading)
            size_t n_pixels = width * height;
            float min_luminance = 0.0f;
            float max_luminance = 0.0f;
            float sum_log_luminance = 0.0f;

            for (size_t i = 0; i < n_pixels; i++) {
                float* pixel_ptr = buffer + (i * 3);  // move forward 3 channels

                auto color = glm::vec3(pixel_ptr[0], pixel_ptr[1], pixel_ptr[2]);
                auto luminance = glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f));

                min_luminance = std::min(min_luminance, luminance);
                max_luminance = std::max(max_luminance, luminance);
                sum_log_luminance += std::max(log(luminance + 0.00001f), 0.0f);  // avoid `log(0)` on black pixels
            }

            float log_average_luminance = exp(sum_log_luminance / n_pixels);

            CORE_TRACE("Minimum luminance of the HDR image is: {0}", min_luminance);
            CORE_TRACE("Maximum luminance of the HDR image is: {0}", max_luminance);
            CORE_TRACE("Log average luminance of the HDR image is: {0}", log_average_luminance);
            
            pixels.reset(reinterpret_cast<uint8_t*>(buffer));
        }

        else {
            CORE_ASERT(read_channels <= 4, "You can only force read up to 4 channels!");
            uint8_t* buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, read_channels);

            if (buffer == nullptr) {
                CORE_ERROR("Failed to load image: {0}", filepath);
                CORE_ERROR("STBI failure reason: {0}", stbi_failure_reason());
                return;
            }

            pixels.reset(buffer);
        }

        CORE_ASERT(n_channels <= 4, "Unexpected image format with {0} channels...", n_channels);

        if (pixels == nullptr) {
            throw std::runtime_error("Unable to claim image data from: " + filepath);
        }
    }

    Image::Image(Image&& other) noexcept {
        *this = std::move(other);
    }

    Image& Image::operator=(Image&& other) noexcept {
        if (this != &other) {
            pixels = nullptr;
            std::swap(pixels, other.pixels);
            std::swap(width, other.width);
            std::swap(height, other.height);
            std::swap(is_hdr, other.is_hdr);
            std::swap(n_channels, other.n_channels);
        }

        return *this;
    }

    bool Image::IsHDR() const {
        return is_hdr;
    }

    GLuint Image::GetWidth() const {
        return static_cast<GLuint>(width);
    }

    GLuint Image::GetHeight() const {
        return static_cast<GLuint>(height);
    }

    GLenum Image::GetFormat() const {
        if (is_hdr) {
            return GL_RGB;
        }

        switch (n_channels) {
            case 1:  return GL_RED;  // greyscale
            case 2:  return GL_RG;   // greyscale + alpha
            case 3:  return GL_RGB;
            case 4:  return GL_RGBA;
            default: return 0;
        }
    }

    GLenum Image::GetInternalFormat() const {
        if (is_hdr) {
            return GL_RGB16F;
        }

        switch (n_channels) {
            case 1:  return GL_R8;   // greyscale
            case 2:  return GL_RG8;  // greyscale + alpha
            case 3:  return GL_RGB8;
            case 4:  return GL_RGBA8;
            default: return 0;
        }
    }

    template<typename T>
    const T* Image::GetPixels() const {
        return reinterpret_cast<const T*>(pixels.get());
    }

    // explicit template functon instantiation
    template const uint8_t* Image::GetPixels() const;
    template const float* Image::GetPixels() const;
}
