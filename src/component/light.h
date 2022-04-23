/* 
   the collection of light classes here merely provide a container for storing light
   relevant attributes and a set of query functions, there's nothing special to look
   at here. These light data is intended to be consumed by buffer objects like UBOs
   and SSBOs, and the real part of lighting calculations are all done in the shaders
*/

#pragma once

#include <limits>
#include <glm/glm.hpp>
#include "component/component.h"

namespace component {

    class Light : public Component {
      public:
        glm::vec3 color;
        float intensity;

        Light(const glm::vec3& color, float intensity = 1.0f)
            : Component(), color(color), intensity(intensity) {}
    };

    class DirectionLight : public Light {
      public:
        using Light::Light;
    };

    class PointLight : public Light {
      public:
        float linear, quadratic;
        float range = std::numeric_limits<float>::max();
        
        using Light::Light;

        void SetAttenuation(float linear, float quadratic);
        float GetAttenuation(float distance) const;
    };

    class Spotlight : public Light {
      public:
        float inner_cutoff;  // angle in degrees at the base of the inner cone
        float outer_cutoff;  // angle in degrees at the base of the outer cone
        float range = std::numeric_limits<float>::max();

        using Light::Light;

        void SetCutoff(float range, float inner_cutoff = 15.0f, float outer_cutoff = 30.0f);
        float GetInnerCosine() const;
        float GetOuterCosine() const;
        float GetAttenuation(float distance) const;
    };

    class AreaLight : public Light {
      public:
        using Light::Light;

        // TODO: to be implemented, using Bezier curves sampling and LTC
    };

    class VolumeLight : public Light {
      public:
        using Light::Light;

        // TODO: to be implemented, requires volumetric path tracing
    };

}