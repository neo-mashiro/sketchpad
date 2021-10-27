#include "pch.h"

#include <random>
#include "utils/math.h"

namespace utils::math {

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

}
