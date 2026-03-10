#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {

	struct Scene;
	struct UniformBuffer;

	REFLECTED_STRUCT(GlobalSettings) {

		GlobalSettings();

		glm::mat4 projview              = {};
		glm::mat4 prev_projview         = {};
		glm::mat4 sun_projview[4]       = {};
		glm::vec4 csm_splits            = {};
		glm::vec3 sun_direction         = { .5f, .5f, .5f };
		glm::vec4 sun_color             = { 1.f, 1.f, 1.f, 1.f };
		float     sun_intensity         = 6.0f;
		glm::vec4 sun_ambient_color     = { 1.f, 1.f, 1.f, 1.f };
		float     sun_ambient_intensity = 0.0f;
		glm::vec3 cam_position          = {};
		// TAA
		bool      taa_enabled           = true;
		float     taa_blend             = 0.9f;
		bool      taa_animated          = true; // Calculate velocity of animated object for better TAA effect
		// Post-processing
		float     pp_exposure           = 1.0f;
		float     pp_gamma              = 2.2f;
		glm::vec4 pp_tint               = { 1.f, 1.f, 1.f, 1.f };
		// Bloom
		bool      pp_bloom_enabled      = true;
		float     pp_bloom_threshold    = 1.0f;
		float     pp_bloom_intensity    = 0.5f;
		float     pp_bloom_knee         = 0.1f;
		// Shadow map
		float     sm_near               = 1.0f;
		float     sm_far                = 100.0f;
		float     sm_ortho_size         = 40.0f;
		// Animation
		bool      anim_enabled          = true;
		// Scripting
		bool      script_enabled        = false;

		// Post-processing Volume support
		void set_override_postprocess(
			float     pp_exposure,
			float     pp_gamma,
			glm::vec4 pp_tint,
			bool      pp_bloom_enabled,
			float     pp_bloom_threshold,
			float     pp_bloom_intensity,
			float     pp_bloom_knee
		);

		void flush();
		void reset_override();
		void bind();
		void unbind();
		uint32_t get_binding() const;

	private:

		struct PostProcessState {
			float     pp_exposure;
			float     pp_gamma;
			glm::vec4 pp_tint;
			bool      pp_bloom_enabled;
			float     pp_bloom_threshold;
			float     pp_bloom_intensity;
			float     pp_bloom_knee;
		};

		bool m_has_pp_override = false;
		PostProcessState m_pp_override = {};

		std::shared_ptr<UniformBuffer> m_global_buffer = nullptr;

		struct alignas(16) GlobalConstants {
			glm::mat4 projview;
			glm::mat4 prev_projview;
			glm::mat4 sun_projview[4];
			glm::vec4 csm_splits;
			glm::vec4 sun_direction;
			glm::vec4 sun_intensity;
			glm::vec4 sun_ambient;
			glm::vec4 cam_position;
			float     taa_enabled;
			float     taa_blend;
			float     pp_exposure;
			float     pp_gamma;
			glm::vec4 pp_tint;
			float     pp_bloom_enabled;
			float     pp_bloom_threshold;
			float     pp_bloom_intensity;
			float     pp_bloom_knee;
		} m_data = {};

	};

	REFLECTED_FIELD(GlobalSettings, sun_direction,         FF_Default, "[drag]step=0.01,group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_color,             FF_Default, "[color]group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_intensity,         FF_Default, "[drag]min=0.0,group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_color,     FF_Default, "[color]group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_intensity, FF_Default, "[drag]min=0.0,group=sun")
	REFLECTED_FIELD(GlobalSettings, taa_enabled,           FF_Default, "group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_blend,             FF_Default, "[slider]min=0.0,max=1.0,group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_animated,          FF_Default, "group=taa")
	REFLECTED_FIELD(GlobalSettings, pp_exposure,           FF_Default, "[drag]min=0.0,group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_gamma,              FF_Default, "[drag]min=0.0,group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_tint,               FF_Default, "[color]group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_bloom_enabled,      FF_Default, "group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_bloom_threshold,    FF_Default, "[drag]min=0.0,group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_bloom_intensity,    FF_Default, "[drag]min=0.0,group=postprocess")
	REFLECTED_FIELD(GlobalSettings, pp_bloom_knee,         FF_Default, "[drag]min=0.0,group=postprocess")
	REFLECTED_FIELD(GlobalSettings, sm_near,               FF_Default, "group=shadow")
	REFLECTED_FIELD(GlobalSettings, sm_far,                FF_Default, "group=shadow")
	REFLECTED_FIELD(GlobalSettings, sm_ortho_size,         FF_Default, "group=shadow")
	REFLECTED_FIELD(GlobalSettings, anim_enabled,          FF_Default, "group=system")
	REFLECTED_FIELD(GlobalSettings, script_enabled,        FF_Default, "group=system")

}
