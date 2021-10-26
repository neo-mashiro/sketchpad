#pragma once

#include <limits>
#include <glm/glm.hpp>
#include "components/component.h"

namespace components {

    class Light : public Component {
      public:
        glm::vec3 color;
        float intensity;

        Light(const glm::vec3& color, float intensity = 1.0f)
            : color(color), intensity(intensity) {}
    };

    class DirectionLight : public Light {
      public:
        using Light::Light;

        DirectionLight(DirectionLight&& other) = default;
        DirectionLight& operator=(DirectionLight&& other) = default;
    };

    class PointLight : public Light {
      public:
        float linear;
        float quadratic;
        float range = std::numeric_limits<float>::max();
        
        using Light::Light;
        
        void SetAttenuation(float linear, float quadratic);
        float GetAttenuation(float distance) const;

        PointLight(PointLight&& other) = default;
        PointLight& operator=(PointLight&& other) = default;
    };

    class Spotlight : public Light {
      private:
        float inner_cutoff;  // angle in degrees at the base of the inner cone
        float outer_cutoff;  // angle in degrees at the base of the outer cone

      public:
        float range = std::numeric_limits<float>::max();

        using Light::Light;

        void SetCutoff(float range, float inner_cutoff = 15.0f, float outer_cutoff = 30.0f);
        float GetInnerCosine() const;
        float GetOuterCosine() const;
        float GetAttenuation(float distance) const;

        Spotlight(Spotlight&& other) = default;
        Spotlight& operator=(Spotlight&& other) = default;
    };

    class AreaLight : public Light {
      public:
        using Light::Light;

        // to be implemented

        AreaLight(AreaLight&& other) = default;
        AreaLight& operator=(AreaLight&& other) = default;
    };

    class VolumeLight : public Light {
      public:
        using Light::Light;

        // to be implemented

        VolumeLight(VolumeLight&& other) = default;
        VolumeLight& operator=(VolumeLight&& other) = default;
    };
}
