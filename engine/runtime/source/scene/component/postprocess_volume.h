#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {

	REFLECTED_COMPONENT(PostprocessVolumeComponent) {
		bool enabled = true;
		bool is_global = false;
		float priority = 0.0f;
		float blend_distance = 1.0f;

		// Overrides
		bool override_exposure = false;
		float exposure = 1.0f;

		bool override_gamma = false;
		float gamma = 2.2f;

		bool override_tint = false;
		glm::vec4 tint = { 1.f, 1.f, 1.f, 1.f };

		bool override_bloom_enabled = false;
		bool bloom_enabled = true;

		bool override_bloom_threshold = false;
		float bloom_threshold = 1.0f;

		bool override_bloom_intensity = false;
		float bloom_intensity = 0.5f;

		bool override_bloom_knee = false;
		float bloom_knee = 0.1f;
	};

	REFLECTED_FIELD(PostprocessVolumeComponent, enabled,                  FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, is_global,                FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, priority,                 FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, blend_distance,           FF_Default, "[drag]min=0.0")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_exposure,        FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, exposure,                 FF_Default, "[drag]min=0.0")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_gamma,           FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, gamma,                    FF_Default, "[drag]min=0.0")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_tint,            FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, tint,                     FF_Default, "[color]")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_bloom_enabled,   FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, bloom_enabled,            FF_Default)

	REFLECTED_FIELD(PostprocessVolumeComponent, override_bloom_threshold, FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, bloom_threshold,          FF_Default, "[drag]min=0.0")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_bloom_intensity, FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, bloom_intensity,          FF_Default, "[drag]min=0.0")

	REFLECTED_FIELD(PostprocessVolumeComponent, override_bloom_knee,      FF_Default)
	REFLECTED_FIELD(PostprocessVolumeComponent, bloom_knee,               FF_Default, "[drag]min=0.0")
}
