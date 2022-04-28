#pragma once

#include <limits>
#include <type_traits>
#include <glm/glm.hpp>

namespace utils::math {

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static constexpr bool IsPowerOfTwo(T value) {  // implicitly inline
        return value != 0 && (value & (value - 1)) == 0;
    }

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    [[nodiscard]] T RandomGenerator();

    bool Equals(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.000001f);
    bool Equals(const glm::quat& a, const glm::quat& b, float epsilon = 0.000001f);
    bool Equals(float a, float b, float tolerance = std::numeric_limits<float>::epsilon());

    float LinearPercent(float a, float b, float t);
    float Lerp(float a, float b, float t);
    float SmoothStep(float a, float b, float t);
    float SmootherStep(float a, float b, float t);

    float EasePercent(float percent_per_second, float delta_time);
    float EaseFactor(float sharpness, float delta_time);
    float Bounce(float x, float k);

    glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, float t);
    glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t);
    glm::vec4 Lerp(const glm::vec4& a, const glm::vec4& b, float t);
    glm::quat Slerp(const glm::quat& a, const glm::quat& b, float t);
    glm::quat SlerpRaw(const glm::quat& a, const glm::quat& b, float t);

    glm::vec3 HSL2RGB(float h, float s, float l);
    glm::vec3 HSV2RGB(float h, float s, float v);
    glm::vec3 HSL2RGB(const glm::vec3& hsl);
    glm::vec3 HSV2RGB(const glm::vec3& hsv);

}
