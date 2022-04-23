/* 
   a helper class for reading and managing image resources, use this class to load files
   from local disk. Given an absolute filepath, it reads in pixel value one by one, then
   store them in a unique pointer that manages and claims ownership of the pixels data.
   the major consumer of this class is the texture class, on construct, a texture will
   create an image and use its buffer pointer to fill the texture storage, w/o having to
   worry about how to release the pointer properly (which this class will do). Other than
   building textures, you may also want to use this class in the resource manager, so the
   pixels data can be shared among multiple scenes. Don't use it elsewhere or in a global
   scope, as images can often consume a very large piece of space in memory.

   the `channels` parameter in constructor forces the number of channels to be read from
   the file, by default it is 0, in which case all channels available will be read. Users
   can also explicitly specify a number between 1 ~ 4, but if the image doesn't have that
   many channels, the extra ones will be filled with 0. Note that this parameter does not
   apply to HDR images, for HDRI we will always force read 4 channels GL_RGBA. HDRIs are
   normally used for rendering skybox and IBL, they are opaque in all cases so the extra
   alpha channel is pointless. However, we'll use image load store (ILS) a lot which only
   accepts RG and RGBA in OpenGL 4.6, as such we have to stick with the GL_RGBA format.

   `GetPixels()` returns a `const T*` pointer to the underlying pixels data, this access
   is made read-only to protect data integrity. The generic type `T` can be either 8-bit
   `uint8_t` or 32-bit `float`, depending on whether or not the image is in high dynamic
   range. While this pointer can tell us where data is stored in memory, the size of the
   data is determined by the image's width, height as well as internal format.

   # file formats

   this class is able to handle most image types supported by `stb`, but 3D volumetric
   images like Nifti is beyond the scope of our demo (supported in my other C# project).
   once the image is loaded, users should expect the pixels data to follow the standards
   listed below, notice the differences between non-HDR and HDR images:

   # standard LDR

    > 8 bits per channel (a.k.a component), 1~4 channels per pixel
    > 1 channel = greyscale, 2 channels = greyscale + alpha
    > 3 channels = red + green + blue (RGB), 4 channels = RGB + alpha (RGBA)
    > color values are stored as fixed-point integers from 0 to 255
    > color values are stored in sRGB space, may require gamma correction (albedo maps)

   # standard HDR

    > ends with the extension ".hdr" or ".exr", typically used as environment cubemaps
    > used to represent colors over a much wider dynamic range (very bright or dark)
    > pixels are stored in 4-channel RGBE, 8 bits per channel (E: shared exponent)
    > pixels are read in as floating-point values, 32 bits per channel RGB, no alpha
    > pixels are read in as linear values in linear color space (i.e. gamma compressed)

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
   of HDR, we will stick with GL_RGB and GL_RGB16F. Using 16-bit floats is usually good
   enough, a 32-bit precision would be overkill, in most cases, there won't be visible
   distinction on a 1080p or smaller window screen.

   to summarize, our definition of standard is 8 bits per channel, up to 4 channels, no
   color space convertion. Even if some PNG files offer 16 ~ 48 bits per channel, we'll
   only read them in as 8 bits to align with the standard.

   # about over exposure

   note that overbright pixels in the HDR image can cause problems to the IBL computation
   in the worst case, a few infinite values could make the irradiance map and prefiltered
   environment map completely unusable, no matter what sampling method is being used, the
   infinity values are going to overshadow all other pixels and dominate the entire image

   this problem can also be observed in engines like Filament or Babylon.js, a rasterizer
   pipeline often cannot handle it, because that would require the IBL pre-computation to
   know the luminance range of an image at runtime and also need an algorithm for it. For
   the sake of adjusting exposure values, we could use the `picturenaut` HDR image editor
   which is a free software able to convert between HDRI formats (tiff, exr and hdr) and
   tonemap a HDRI to a high dynamic range that's narrower and more affordable, the latter
   is called "dynamic compression" in the menu, using the default values is good enough.

   for free to use HDRIs, check out https://www.ihdri.com/ & https://polyhaven.com/hdris
   for free 360 panorama images, check out https://www.flickr.com/groups/equirectangular/
   you can also stitch HDR panoramas in Photoshop or create one from scratch in Blender.
*/

#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>

namespace utils {

    class Image {
      private:
        GLint width, height, n_channels;
        bool is_hdr;

        struct deleter {
            void operator()(uint8_t* buffer);
        };

        std::unique_ptr<uint8_t, deleter> pixels;  // with `stb` custom deleter

      public:
        Image(const std::string& filepath, GLuint channels = 0, bool flip = false);

        Image(const Image&) = delete;  // `std::unique_ptr` is move-only
        Image& operator=(const Image&) = delete;
        Image(Image&& other) noexcept = default;
        Image& operator=(Image&& other) noexcept = default;

        bool IsHDR() const;
        GLuint Width() const;
        GLuint Height() const;
        GLenum Format() const;
        GLenum IFormat() const;

        template<typename T>
        const T* GetPixels() const;
    };

}
