#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

#include <cstddef>

namespace z1 {

	struct Scene;
	struct UniformBuffer;

	enum struct API RenderMode : int {
		Forward = 0,
		Deferred = 1,
	};

	REFLECT_ENUM(RenderMode, Forward)
	REFLECT_ENUM(RenderMode, Deferred)

	enum struct API AOMode : int {
		SSAO = 0,
		GTAO = 1,
	};

	REFLECT_ENUM(AOMode, SSAO)
	REFLECT_ENUM(AOMode, GTAO)

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
		float     sun_ambient_intensity = 0.25f;
		glm::vec3 cam_position          = {};
		// TAA
		bool      taa_enabled           = true;
		float     taa_blend             = 0.1f;  // New-frame weight (inverted from old 0.9 history weight)
		float     taa_variance_scale    = 1.0f;  // Higher = more aggressive history rejection in high-variance areas
		float     taa_clip_gamma        = 1.0f;  // Variance expansion for AABB clip (UE4-style)
		glm::vec2 taa_jitter_uv         = {};     // Per-frame jitter offset in UV space (non-serialized)
		bool      taa_sharpen_enabled   = true;
		float     taa_sharpen_strength  = 0.3f;
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
		// Ambient Occlusion
		bool      ao_enabled            = true;
		AOMode    ao_type               = AOMode::GTAO;     // 0 = SSAO, 1 = GTAO
		float     ao_radius             = 1.0f;
		float     ao_intensity          = 1.0f;
		float     ao_power              = 1.5f;
		float     ao_bias               = 0.025f;
		bool      ao_blur_enabled       = true;
		float     ao_blur_strength      = 1.0f;
		// Screen-space reflections (deferred only)
		bool      ssr_enabled           = false;
		float     ssr_intensity         = 0.6f;
		float     ssr_max_distance      = 30.0f;
		float     ssr_thickness         = 0.15f;
		float     ssr_stride            = 0.07f;
		float     ssr_max_steps         = 128.0f;
		float     ssr_jitter_strength   = 0.25f;
		// SkyLight runtime parameters (shared by forward/deferred)
		glm::vec4 sky_params            = {}; // x=rotation, y=intensity, z=mip_level, w=specular_max_mip
		glm::vec4 sky_sh[9]             = {};
		// Animation
		bool      anim_enabled          = true;
		// Scripting
		bool      script_enabled        = false;
		// Render mode
		RenderMode render_mode          = RenderMode::Deferred;

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
			// TAA upgrade (appended at end to preserve existing layout alignment)
			float     taa_variance_scale;
			float     taa_clip_gamma;
			float     taa_jitter_u;
			float     taa_jitter_v;
			float     taa_sharpen_enabled;
			float     taa_sharpen_strength;
			// Ambient Occlusion (appended at end to preserve existing layout alignment)
			float     ao_enabled;
			float     ao_type;
			float     ao_radius;
			float     ao_intensity;
			float     ao_power;
			float     ao_bias;
			float     ao_blur_enabled;
			float     ao_blur_strength;
			// Screen-space reflection (appended at end to preserve existing layout alignment)
			float     ssr_enabled;
			float     ssr_intensity;
			float     ssr_max_distance;
			float     ssr_thickness;
			float     ssr_stride;
			float     ssr_max_steps;
			float     ssr_jitter_strength;
			// SkyLight (appended at end to preserve existing layout alignment)
			// std140 requires vec4 members to start on 16-byte boundaries, but
			// glm::vec4 has 4-byte alignment in this build. Pad explicitly so
			// sky_params lands at offset 608, matching uniforms.glsl.
			float     sky_padding[3];
			glm::vec4 sky_params;
			glm::vec4 sky_sh[9];
		} m_data = {};

		// Compile-time std140 alignment verification for the UBO mirror above
		// (must match the GLSL Global block in include/uniforms.glsl):
		// vec4 members need 16-byte alignment, mat4 members 64-byte alignment
		// (std140 only requires 16, but every mat4 here is 64-aligned).
		#define Z1_UBO_ALIGN_CHECK(field, alignment) \
			static_assert(offsetof(GlobalConstants, field) % (alignment) == 0, \
				"GlobalConstants std140 alignment mismatch: " #field)
		Z1_UBO_ALIGN_CHECK(projview, 64);
		Z1_UBO_ALIGN_CHECK(prev_projview, 64);
		Z1_UBO_ALIGN_CHECK(sun_projview, 64);
		Z1_UBO_ALIGN_CHECK(csm_splits, 16);
		Z1_UBO_ALIGN_CHECK(sun_direction, 16);
		Z1_UBO_ALIGN_CHECK(sun_intensity, 16);
		Z1_UBO_ALIGN_CHECK(sun_ambient, 16);
		Z1_UBO_ALIGN_CHECK(cam_position, 16);
		Z1_UBO_ALIGN_CHECK(pp_tint, 16);
		Z1_UBO_ALIGN_CHECK(sky_params, 16);
		Z1_UBO_ALIGN_CHECK(sky_sh, 16);
		#undef Z1_UBO_ALIGN_CHECK
		// Array strides must match std140 (vec4: 16, mat4: 64).
		static_assert(offsetof(GlobalConstants, sun_projview[1]) - offsetof(GlobalConstants, sun_projview[0]) == 64,
			"GlobalConstants std140 stride mismatch: sun_projview");
		static_assert(offsetof(GlobalConstants, sky_sh[1]) - offsetof(GlobalConstants, sky_sh[0]) == 16,
			"GlobalConstants std140 stride mismatch: sky_sh");
		static_assert(sizeof(GlobalConstants) == 768, "GlobalConstants std140 block size mismatch");

	};

	REFLECTED_FIELD(GlobalSettings, sun_direction,         FF_Default, "[drag]step=0.01,group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_color,             FF_Default, "[color]group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_intensity,         FF_Default, "[drag]min=0.0,group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_color,     FF_Default, "[color]group=sun")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_intensity, FF_Default, "[drag]min=0.0,group=sun")
	REFLECTED_FIELD(GlobalSettings, taa_enabled,           FF_Default, "group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_blend,             FF_Default, "[slider]min=0.0,max=1.0,group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_variance_scale,    FF_Default, "[drag]min=0.0,group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_clip_gamma,        FF_Default, "[drag]min=0.0,group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_sharpen_enabled,   FF_Default, "group=taa")
	REFLECTED_FIELD(GlobalSettings, taa_sharpen_strength,  FF_Default, "[slider]min=0.0,max=2.0,group=taa")
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
	REFLECTED_FIELD(GlobalSettings, ao_enabled,            FF_Default, "group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_type,               FF_Default, "group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_radius,             FF_Default, "[drag]min=0.0,group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_intensity,          FF_Default, "[drag]min=0.0,max=2.0,group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_power,              FF_Default, "[drag]min=0.0,group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_bias,               FF_Default, "[drag]min=0.0,group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_blur_enabled,       FF_Default, "group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ao_blur_strength,      FF_Default, "[drag]min=0.0,max=4.0,group=ambient_occlusion")
	REFLECTED_FIELD(GlobalSettings, ssr_enabled,           FF_Default, "group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_intensity,         FF_Default, "[slider]min=0.0,max=2.0,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_max_distance,      FF_Default, "[drag]min=0.0,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_thickness,         FF_Default, "[drag]min=0.0,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_stride,            FF_Default, "[drag]min=0.01,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_max_steps,         FF_Default, "[drag]min=1.0,max=256.0,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, ssr_jitter_strength,   FF_Default, "[slider]min=0.0,max=1.0,group=screen_space_reflection")
	REFLECTED_FIELD(GlobalSettings, anim_enabled,          FF_Default, "group=system")
	REFLECTED_FIELD(GlobalSettings, script_enabled,        FF_Default, "group=system")
	REFLECTED_FIELD(GlobalSettings, render_mode,           FF_Default, "group=system")

}
