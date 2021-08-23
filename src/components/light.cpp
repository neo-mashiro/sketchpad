#include "pch.h"

#include "core/log.h"
#include "components/light.h"

namespace components {

    void PointLight::SetAttenuation(float linear, float quadratic) {
        CORE_ASERT(linear > 0, "The linear attenuation factor must be positive ...");
        CORE_ASERT(quadratic > 0, "The quadratic attenuation factor must be positive ...");

        this->linear = linear;
        this->quadratic = quadratic;

        // find the max range by solving the quadratic equation when attenuation is <= 0.01
        float a = quadratic;
        float b = linear;
        float c = -100.0f;
        float delta = b * b - 4.0f * a * c;

        CORE_ASERT(delta > 0.0f, "You can never see this line, it is mathematically impossible...");
        range = c / (-0.5f * (b + sqrt(delta)));  // Muller's method
    }

    float PointLight::GetAttenuation(float distance) const {
        // our point light follows the inverse-square law, so the attenuation is inversely
        // proportional to the square of distance. While it does not have a real concept of
        // "range", we can calculate a range threshold where the attenuation becomes a very
        // small number (such as 0.01), and return 0 if the distance is out of this range.

        CORE_ASERT(distance >= 0.0f, "Distance to the light source cannot be negative ...");

        return distance >= range ? 0.0f :
            1.0f / (1.0f + linear * distance + quadratic * pow(distance, 2.0f));
    }

    void Spotlight::SetCutoff(float range, float inner_cutoff, float outer_cutoff) {
        CORE_ASERT(range > 0, "The spotlight range must be positive ...");
        CORE_ASERT(inner_cutoff > 0, "The inner cutoff angle must be positive ...");
        CORE_ASERT(outer_cutoff > 0, "The outer cutoff angle must be positive ...");

        this->range = range;
        this->inner_cutoff = inner_cutoff;
        this->outer_cutoff = outer_cutoff;
    }

    float Spotlight::GetInnerCosine() const {
        return glm::cos(glm::radians(inner_cutoff));
    }

    float Spotlight::GetOuterCosine() const {
        return glm::cos(glm::radians(outer_cutoff));
    }

    float Spotlight::GetAttenuation(float distance) const {
        // to keep it simple and avoid too many parameters, our spotlight does not follow the
        // inverse-square law, instead, the attenuation uses a linear falloff. Despite that,
        // we do have a fade-out effect from the inner to the edges of the outer cone, so
        // the overall effect should still be realistic enough without loss of fidelity.

        CORE_ASERT(distance >= 0.0f, "Distance to the light source cannot be negative ...");

        return 1.0f - std::clamp(distance / range, 0.0f, 1.0f);
    }

}
