#pragma once

namespace utils::math {

	uint64_t RandomUInt64();
	float_t RandomFloat01();

	template<typename T>
	static constexpr bool IsPowerOfTwo(T value) {
		return value != 0 && (value & (value - 1)) == 0;
	}

}
