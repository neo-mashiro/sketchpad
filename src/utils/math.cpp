#include "pch.h"

#include <random>
#include "utils/math.h"

namespace utils::math {

    using namespace glm;

    static std::random_device device;
    static std::mt19937 engine_32(device());
    static std::mt19937_64 engine_64(device());

    static std::uniform_int_distribution<uint64_t> uint64_t_distributor;
    static std::uniform_real_distribution<> float01_distributor(0, 1);

    uint64_t RandomUInt64() {
        return uint64_t_distributor(engine_64);
    }

    float_t RandomFloat01() {
        double t = float01_distributor(engine_32);
        return static_cast<float>(t);
    }

    float Lerp(float a, float b, float t) {
        if (t < 0.0f || t > 1.0f) return 0.0f;
        return a * (1.0f - t) + b * t;
    }

    vec3 Lerp(vec3 a, vec3 b, float t) {
        return vec3(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t));
    }

    float Gaussian(float x, float sigma) {
        float sigma2 = sigma * sigma;
        double coefficient = 1.0 / (glm::two_pi<double>() * sigma2);
        double exponent = -(x * x) / (2.0 * sigma2);
        return static_cast<float>(coefficient * exp(exponent));
    }
}
