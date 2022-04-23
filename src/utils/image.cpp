#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#ifdef STB_IMAGE_WRITE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include "core/log.h"
#include "utils/ext.h"
#include "utils/image.h"

namespace utils {

    void Image::deleter::operator()(uint8_t* buffer) {
        if (buffer != nullptr) {
            stbi_image_free(buffer);
        }
    }

    Image::Image(const std::string& filepath, GLuint channels, bool flip) : width(0), height(0), n_channels(0) {
        stbi_set_flip_vertically_on_load(flip);

        // supported file extensions (will support ".psd", ".tga" and ".gif" in the future)
        const std::vector<std::string> extensions { ".jpg", ".png", ".jpeg", ".bmp", ".hdr", ".exr" };

        // check valid file extension
        auto ext = filepath.substr(filepath.rfind("."));
        if (ranges::find(extensions, ext) == extensions.end()) {
            CORE_ERROR("Image file format is not supported: {0}", filepath);
            return;
        }

        CORE_INFO("Loading image from: {0}", filepath);
        this->is_hdr = stbi_is_hdr(filepath.c_str());

        if (is_hdr) {
            float* buffer = stbi_loadf(filepath.c_str(), &width, &height, &n_channels, 4);

            if (buffer == nullptr) {
                CORE_ERROR("Failed to load image: {0}", filepath);
                CORE_ERROR("STBI failure reason: {0}", stbi_failure_reason());
                return;
            }

            // report HDR image statistics before the buffer pointer is reset
            size_t n_pixels = width * height;
            float min_luminance = std::numeric_limits<float>::max();
            float max_luminance = std::numeric_limits<float>::min();
            float sum_log_luminance = 0.0f;

            for (size_t i = 0; i < n_pixels; i++) {
                const float* pixel_ptr = buffer + (i * 3);  // move forward 3 channels

                auto color = glm::vec3(pixel_ptr[0], pixel_ptr[1], pixel_ptr[2]);
                auto luminance = glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f));

                min_luminance = std::min(min_luminance, luminance);
                max_luminance = std::max(max_luminance, luminance);
                sum_log_luminance += std::max(log(luminance + 0.00001f), 0.0f);  // avoid division by zero
            }

            float log_average_luminance = exp(sum_log_luminance / n_pixels);

            CORE_TRACE("HDR image luminance report:");
            CORE_TRACE("------------------------------------------------------------------------");
            CORE_DEBUG("min: {0}, max: {1}, log average: {2}", min_luminance, max_luminance, log_average_luminance);
            CORE_TRACE("------------------------------------------------------------------------");

            float luminance_diff = max_luminance - min_luminance;
            if (luminance_diff > 10000.0f) {
                CORE_WARN("Input HDR image is too bright, some pixels have values close to infinity!");
                CORE_WARN("This can lead to serious artifact in IBL or even completely white images!");
                CORE_WARN("Please use a different image or manually adjust the exposure values (EV)!");
            }
            
            pixels.reset(reinterpret_cast<uint8_t*>(buffer));
        }

        else {
            CORE_ASERT(channels <= 4, "You can only force read up to 4 channels!");
            uint8_t* buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, channels);

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

    bool Image::IsHDR() const {
        return is_hdr;
    }

    GLuint Image::Width() const {
        return static_cast<GLuint>(width);
    }

    GLuint Image::Height() const {
        return static_cast<GLuint>(height);
    }

    GLenum Image::Format() const {
        if (is_hdr) {
            return GL_RGBA;
        }

        switch (n_channels) {
            case 1:  return GL_RED;  // greyscale
            case 2:  return GL_RG;   // greyscale + alpha
            case 3:  return GL_RGB;
            case 4:  return GL_RGBA;
            default: return 0;
        }
    }

    GLenum Image::IFormat() const {
        if (is_hdr) {
            return GL_RGBA16F;
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

    // explicit template function instantiation
    template const uint8_t* Image::GetPixels<uint8_t>() const;
    template const float* Image::GetPixels<float>() const;

}
