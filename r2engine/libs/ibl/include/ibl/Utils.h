#ifndef __IBL_UTILS_H__
#define __IBL_UTILS_H__

#include "glm/glm.hpp"

namespace r2::ibl
{

	template <typename T>
	static inline constexpr T log4(T x)
	{
		return std::log2(x) * T(0.5);
	}

	inline bool isPOT(size_t x) {
		return !(x & (x - 1));
	}

	inline glm::vec2 Hammersley(uint32_t i, float iN)
	{
		constexpr float tof = 0.5f / 0x80000000U;
		uint32_t bits = i;
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return { i * iN, bits * tof };
	}
}


#endif