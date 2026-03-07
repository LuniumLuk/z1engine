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
		// Post-processing
		float     pp_exposure           = 1.0f;
		float     pp_gamma              = 2.2f;
		glm::vec4 pp_tint               = { 1.f, 1.f, 1.f, 1.f };
		// Shadow map
		float     sm_near               = 1.0f;
		float     sm_far                = 100.0f;
		float     sm_ortho_size         = 40.0f;

		void flush();
		void bind();
		void unbind();
		uint32_t get_binding() const;

	private:

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
		} m_data = {};

	};

	REFLECTED_FIELD(GlobalSettings, sun_direction,         FF_Default, "[drag]step=0.01")
	REFLECTED_FIELD(GlobalSettings, sun_color,             FF_Default, "[color]")
	REFLECTED_FIELD(GlobalSettings, sun_intensity,         FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_color,     FF_Default, "[color]")
	REFLECTED_FIELD(GlobalSettings, sun_ambient_intensity, FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(GlobalSettings, taa_enabled,           FF_Default)
	REFLECTED_FIELD(GlobalSettings, taa_blend,             FF_Default, "[slider]min=0.0,max=1.0")
	REFLECTED_FIELD(GlobalSettings, pp_exposure,           FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(GlobalSettings, pp_gamma,              FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(GlobalSettings, pp_tint,               FF_Default, "[color]")
	REFLECTED_FIELD(GlobalSettings, sm_near,               FF_Default)
	REFLECTED_FIELD(GlobalSettings, sm_far,                FF_Default)
	REFLECTED_FIELD(GlobalSettings, sm_ortho_size,         FF_Default)

}
