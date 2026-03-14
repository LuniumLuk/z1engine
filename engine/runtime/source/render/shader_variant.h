#pragma once

#include <cstdint>

namespace z1 {

	namespace ShaderVariant {
		constexpr uint32_t None     = 0;
		constexpr uint32_t GBuffer  = 1 << 0;
		constexpr uint32_t Shadow   = 1 << 1;
		constexpr uint32_t Velocity = 1 << 2;

		// Human-readable name for logging / debugging
		inline const char* bit_name(uint32_t single_bit) {
			switch (single_bit) {
			case GBuffer:  return "VARIANT_GBUFFER";
			case Shadow:   return "VARIANT_SHADOW";
			case Velocity: return "VARIANT_VELOCITY";
			default:       return "UNKNOWN";
			}
		}
	}

}
