#pragma once

#include <memory>
#include <string>
#include <GL/glew.h>

namespace utils {

    /* a helper class for reading and managing image resources, use this class to load files
       from local disk and then feed it to the texture ctor. Given an absolute filepath, it
       reads in pixel value one by one, and then store them in a unique pointer that manages
       and claims exclusive ownership of the pixels data. The major consumer of this class
       is the texture class, on construct, a texture first creates an image from a filepath,
       allocate memory space for the image, retrieve image attributes such as width, height,
       format and internal format to build up texture data from the image buffer pointer,
       and finally discard the image so that its memory will be freed on the CPU side.

       the `read_channels` parameter in the ctor forces the number of channels to read from
       the file, by default it is 0, in which case all channels available will be read. The
       user can also specify it to be 1~4, but if the image does not have that many channels,
       the extra ones will always be 0. Note that this parameter only applies to LDR images,
       for HDR images, we always read 3 channels GL_RGB so it has no effect.

       the `GetPixels()` method returns a raw pointer to the pixels data, this pointer is of
       type `const T*` that provides read-only access, users are not allowed to mutate the
       content of the image. The generic type `T` can be either an 8-bit byte `uint8_t` or a
       32-bit `float`, depending on whether or not this is a high-dynamic-range (HDR) image.
       while this pointer tells us the position of data in memory, the size of the data is
       determined by the width, height of the image as well as its internal format.

       once the image has been loaded, users should expect the pixels data to follow the
       standards below, note the differences between traditional images and HDR images

       ## standard LDR
         -- 8 bits per channel (a.k.a component), 1~4 channels per pixel
         -- 1 channel = greyscale, 2 channels = greyscale + alpha
         -- 3 channels = red + green + blue (RGB), 4 channels = RGB + alpha (RGBA)
         -- color values are stored as fixed-point integers from 0 to 255
         -- color values are stored in sRGB space, may require gamma correction

       ## standard HDR
         -- 32 bits per channel, 3 channels per pixel (RGB, no alpha) 
         -- end with the extension ".hdr", typically used as cubemaps and irradiance maps
         -- used to represent colors over a much wider dynamic range (very bright or dark)
         -- color values are stored as floating-point linear values
         -- color values are stored in linear color space (i.e. gamma compressed)
       
       # color space

       this class does not apply any built-in gamma correction, so the pixels data is always
       stored in its original color space, users are required to handle color space manually.
       in specific, we can work on HDR pixels directly as they are already in linear space,
       but for traditional images, we need to be aware of the type of textures.

       keep in mind that most color textures such as albedo and emission maps are encoded in
       sRGB color space (industry standard), so that users have to raise the color values to
       the power of 2 to work in linear space, then convert back to sRGB space after shading.
       however, textures such as normal, metallic, roughness, opacity, ambient occlussion or
       displacement maps are only meant to store numeric values, they do not represent real
       colors, and are intended to be used directly regardless of their color format.

       # format and internal format

       this class only aims to handle images for textures in general, it does not use any of
       the normalized formats or gamma corrected formats such as GL_RGB8_SNORM and GL_SRGB8.
       for more specialized data formats, users are required to handle them manually.

       to standardize the common rules mentioned above, our image class adopts a consistent
       data format that directly translates to texture format in OpenGL. For 1, 2, 3 and 4
       channels, data format will always be GL_RED, GL_RG, GL_RGB and GL_RGBA, accordingly,
       the internal format will be GL_R8, GL_RG8, GL_RGB8 and GL_RGBA8. For the special case
       of HDR, we use a fixed format of GL_RGB, in pair with an internal format of GL_RGB16F.
       using 16-bit floats is often good enough, a 32-bit precision would be overkill, in
       most cases, there won't be visible distinction on a 1080p or smaller window screen.

       to summarize, our definition of standard is 8 bits per channel, up to 4 channels, no
       color space convertion. Even if some PNG files offer 16 or 32 bits per channel, we'll
       only read them in as 8 bits to align with the standard.
    */

    class Image {
      private:
        GLint width, height, n_channels;
        bool is_hdr;

        // manage the resource using `std::unique_ptr` with a custom deleter
        struct deleter {
            void operator()(uint8_t* buffer);
        };

        std::unique_ptr<uint8_t, deleter> pixels;

      public:
        Image(const std::string& filepath, GLuint read_channels = 0);

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;

        bool IsHDR() const;
        GLuint GetWidth() const;
        GLuint GetHeight() const;
        GLenum GetFormat() const;
        GLenum GetInternalFormat() const;

        template<typename T>
        const T* GetPixels() const;
    };

}
