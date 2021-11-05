#pragma once

namespace utils::math {

	template<typename T>
	static constexpr bool IsPowerOfTwo(T value) {
		return value != 0 && (value & (value - 1)) == 0;
	}

    uint64_t RandomUInt64();

    float_t RandomFloat01();

    float Lerp(float a, float b, float t);

    glm::vec3 Lerp(glm::vec3 a, glm::vec3 b, float t);

    float Gaussian(float x, float sigma);

}
