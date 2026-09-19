#pragma once

#include "core/core.h"

#include <cstring>

// Semantic uniform-block binding points: every program resolves these blocks to the same binding
// point at link time, so per-frame work is reduced to glBindBufferBase only when a buffer changes.
namespace z1::uniform_blocks {

	constexpr uint32_t Global = 0;
	constexpr uint32_t Lights = 1;
	constexpr uint32_t Bones = 2;
	constexpr uint32_t PrevBones = 3;

	// Returns the fixed binding point for a block name, or INVALID_BINDING when not in the table.
	inline uint32_t find(char const* name) {
		if (std::strcmp(name, "Global") == 0) return Global;
		if (std::strcmp(name, "Lights") == 0) return Lights;
		if (std::strcmp(name, "Bones") == 0) return Bones;
		if (std::strcmp(name, "PrevBones") == 0) return PrevBones;
		return INVALID_BINDING;
	}

}
