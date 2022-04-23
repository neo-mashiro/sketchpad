/* 
   samplers are standalone state objects that store the sampling parameters of a texture.
   using samplers, we can effectively separate the sampling state from textures, so that
   a texture can be a clean buffer object that purely holds data. When a sampler is bound
   to a texture unit, its own state will override the internal sampling parameters for a
   texture bound to the same unit (also applies to ILS). In our demo, samplers are mainly
   used to override the default sampling state set by the `Texture` class.

   what makes samplers really powerful is its ability to bind to multiple texture units
   simultaneously, this way, we can configure a sampling state for many textures at once.

   in the case of filtering, keep in mind that a filtering mode will only produce correct
   results if the texture is in linear colorspace. It is very important not to apply any
   convolution filter on a texture that's encoded in sRGB colorspace. Also, note that the
   wrapping mode can lead to sampling artifacts on the edges if not correctly setup, for
   framebuffer textures, make sure that it is set to clamp on the edges, or even better,
   clamp to the border with black as the clear border color.
*/

#pragma once

#include "asset/asset.h"

namespace asset {

    enum class FilterMode : uint8_t {
        Point, Bilinear, Trilinear  // preset samplers
    };

    class Sampler : public IAsset {
      public:
        Sampler(FilterMode mode = FilterMode::Point);
        ~Sampler();

        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;
        Sampler(Sampler&& other) noexcept = default;
        Sampler& operator=(Sampler&& other) noexcept = default;

      public:
        void Bind(GLuint index) const override;
        void Unbind(GLuint index) const override;

        template<typename T>
        void SetParam(GLenum name, T value) const;

        template<typename T>
        void SetParam(GLenum name, const T* value) const;
    };

}