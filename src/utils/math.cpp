#include "pch.h"

#include <cmath>
#include <random>
#include "utils/math.h"

using namespace glm;

namespace utils::math {

    static std::random_device device;
    static std::mt19937 engine_32(device());
    static std::mt19937_64 engine_64(device());

    static std::uniform_int_distribution<uint64_t> uint64_t_distributor;
    static std::uniform_real_distribution<double> float01_distributor(0, 1);

    uint64_t RandomUInt64() {
        return uint64_t_distributor(engine_64);
    }

    float RandomFloat01() {
        double t = float01_distributor(engine_32);
        return static_cast<float>(t);
    }

    bool Equals(const vec3& a, const vec3& b, float epsilon) {
        return glm::length2(a - b) < epsilon;  // using `length2` can save us a `sqrt()` operation
    }

    bool Equals(const quat& a, const quat& b, float epsilon) {
        return abs(glm::dot(a, b) - 1.0f) < epsilon;
    }

    float SmoothStep(float a, float b, float t) {
        t = std::clamp((t - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);  // see https://en.wikipedia.org/wiki/Smoothstep
    }

    float SmootherStep(float a, float b, float t) {
        t = std::clamp((t - a) / (b - a), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);  // see https://en.wikipedia.org/wiki/Smoothstep
    }

    float EasePercent(float percent_per_second, float delta_time) {
        // returns a framerate-independent t for lerp and slerp given the % of distance to cover per second
        return 1.0f - pow(1.0f - percent_per_second, delta_time);
    }

    float EaseFactor(float sharpness, float delta_time) {
        // returns a framerate-independent t for lerp and slerp given the sharpness (opposite of smoothness)
        return 1.0f - exp(-sharpness * delta_time);
    }

    float Lerp(float a, float b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return a * (1.0f - t) + b * t;
    }

    vec3 Lerp(const vec3& a, const vec3& b, float t) {
        return vec3(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t));
    }

    quat Slerp(const quat& a, const quat& b, float t) {
        // with `glm::slerp`, interpolation takes the short path, but there's a chance
        // that `a` will never get close enough to `b`, resulting in an infinite slerp
        t = std::clamp(t, 0.0f, 1.0f);
        return glm::slerp(a, b, t);
    }

    quat SlerpRaw(const quat& a, const quat& b, float t) {
        // with `glm::mix`, interpolation is oriented, an infinite loop deadlock won't
        // occur, and it's safe for rotations > PI, but for humanoid animations where
        // short path is desired, this version won't fit
        t = std::clamp(t, 0.0f, 1.0f);
        return glm::mix(a, b, t);
    }

    vec3 HSL2RGB(float h, float s, float l) {
        // https://en.wikipedia.org/wiki/HSL_and_HSV
        vec3 rgb = clamp(abs(mod(h * 6.0f + vec3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f);
        return l + s * (rgb - 0.5f) * (1.0f - abs(2.0f * l - 1.0f));
    }

    vec3 HSV2RGB(float h, float s, float v) {
        // https://en.wikipedia.org/wiki/HSL_and_HSV
        if (s <= std::numeric_limits<float>::epsilon()) {
            return vec3(v);  // zero saturation = grayscale color
        }

        h = fmodf(h, 1.0f) / (60.0f / 360.0f);
        int i = static_cast<int>(h);

        float f = h - static_cast<float>(i);
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
            case 0: return vec3(v, t, p);
            case 1: return vec3(q, v, p);
            case 2: return vec3(p, v, t);
            case 3: return vec3(p, q, v);
            case 4: return vec3(t, p, v);
            case 5: default: {
                return vec3(v, p, q);
            }
        }
    }

    vec3 HSL2RGB(const vec3& hsl) {
        return HSL2RGB(hsl.x, hsl.y, hsl.z);
    }

    vec3 HSV2RGB(const vec3& hsv) {
        return HSV2RGB(hsv.x, hsv.y, hsv.z);
    }

    float Gaussian(float x, float sigma) {
        float sigma2 = sigma * sigma;
        double coefficient = 1.0 / (glm::two_pi<double>() * sigma2);
        double exponent = -(x * x) / (2.0 * sigma2);
        return static_cast<float>(coefficient * exp(exponent));
    }

}
