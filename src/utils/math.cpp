#include "pch.h"

#include <cmath>
#include <limits>
#include <random>
#include "utils/math.h"

using namespace glm;

namespace utils::math {

    // create random number generator engines and set engine state
    static std::random_device device;               // acquire seed from device()
    static std::mt19937       engine_32(device());  // seed only once at startup
    static std::mt19937_64    engine_64(device());  // seed only once at startup

    // construct static uniform distributions
    static std::uniform_int_distribution<uint32_t> uint32_t_distributor(0, std::numeric_limits<uint32_t>::max());
    static std::uniform_int_distribution<uint64_t> uint64_t_distributor(0, std::numeric_limits<uint64_t>::max());
    static std::uniform_real_distribution<double>  double_t_distributor(0.0, 1.0);

    template<typename T> inline constexpr bool const_false = false;

    // our seed is constant and not relying on `std::chrono::high_resolution_clock` so
    // the generated pseudo-random number sequence is deterministic and reproducible
    template<typename T, typename>
    T RandomGenerator() {
        if constexpr (std::is_same_v<T, uint64_t>) {
            return uint64_t_distributor(engine_64);
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
            return uint32_t_distributor(engine_32);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return double_t_distributor(engine_32);
        }
        else if constexpr (std::is_same_v<T, float>) {
            return static_cast<float>(double_t_distributor(engine_32));
        }
        else {
            static_assert(const_false<T>, "Unsupported integral type T ...");
        }
    }

    // explicit template function instantiation
    template uint64_t RandomGenerator();
    template uint32_t RandomGenerator();
    template double   RandomGenerator();
    template float    RandomGenerator();

    // tests the equality of two vectors under the threshold epsilon
    bool Equals(const vec3& a, const vec3& b, float epsilon) {
        return glm::length2(a - b) < epsilon;  // using `length2` can save us a `sqrt()` operation
    }

    // tests the equality of two quaternions under the threshold epsilon
    // compared to vectors, this epsilon should be more relaxed, o/w it'd be difficult for slerp to converge
    bool Equals(const quat& a, const quat& b, float epsilon) {
        return abs(glm::dot(a, b) - 1.0f) < epsilon;
    }

    // tests if two floats are approximately equal within the tolerance
    bool Equals(float a, float b, float tolerance) {
        return abs(a - b) <= tolerance;
    }

    // returns the linear percent of distance between a and b, this is essentially the CDF of a uniform
    // distribution but can be negative or greater than 1, useful for keyframes timestamp interpolation
    float LinearPercent(float a, float b, float t) {
        return Equals(a, b) ? 1.0f : (t - a) / (b - a);
    }

    // returns the linear blend of two floating-point numbers
    float Lerp(float a, float b, float t) {
        return glm::lerp(a, b, t);
    }

    // performs a smooth Hermite interpolation between two numbers, returns a percent in range [0, 1]
    // see https://en.wikipedia.org/wiki/Smoothstep
    float SmoothStep(float a, float b, float t) {
        t = std::clamp((t - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // performs a second-order smooth interpolation between two numbers, returns a percent in range [0, 1]
    // see https://en.wikipedia.org/wiki/Smoothstep
    float SmootherStep(float a, float b, float t) {
        t = std::clamp((t - a) / (b - a), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // returns a framerate-independent t for lerp/slerp given the percent of distance to cover per second
    float EasePercent(float percent_per_second, float delta_time) {
        return 1.0f - pow(1.0f - percent_per_second, delta_time);
    }

    // returns a framerate-independent t for lerp/slerp given the level of sharpness (sharp = not smooth)
    float EaseFactor(float sharpness, float delta_time) {
        return 1.0f - exp(-sharpness * delta_time);
    }

    // returns a float that bounces between 0.0 and k as the value of x changes monotonically
    float Bounce(float x, float k) {
        return k - fabs(k - fmodf(x, k * 2));
    }

    // returns the linear blend of two vectors
    vec2 Lerp(const vec2& a, const vec2& b, float t) { return glm::lerp(a, b, t); }
    vec3 Lerp(const vec3& a, const vec3& b, float t) { return glm::lerp(a, b, t); }
    vec4 Lerp(const vec4& a, const vec4& b, float t) { return glm::lerp(a, b, t); }

    // performs a spherical interpolation between two quaternions, takes shortest path
    quat Slerp(const quat& a, const quat& b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return glm::normalize(glm::slerp(a, b, t));
    }

    // performs a spherical interpolation between two quaternions, takes oriented path
    quat SlerpRaw(const quat& a, const quat& b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return glm::normalize(glm::mix(a, b, t));
    }

    // creates RGB color from HSL, see https://en.wikipedia.org/wiki/HSL_and_HSV
    vec3 HSL2RGB(float h, float s, float l) {
        vec3 rgb = clamp(abs(mod(h * 6.0f + vec3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f);
        return l + s * (rgb - 0.5f) * (1.0f - abs(2.0f * l - 1.0f));
    }

    // creates RGB color from HSV, see https://en.wikipedia.org/wiki/HSL_and_HSV
    vec3 HSV2RGB(float h, float s, float v) {
        if (s <= std::numeric_limits<float>::epsilon()) {
            return vec3(v);  // zero saturation = grayscale color
        }

        h = fmodf(h, 1.0f) * 6.0f;
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

}