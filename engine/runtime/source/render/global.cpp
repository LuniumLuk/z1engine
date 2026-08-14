#include "pch.h"

#include "render/global.h"
#include "render/buffer.h"

namespace z1 {

	GlobalSettings::GlobalSettings() {
		m_global_buffer = UniformBuffer::create(nullptr, sizeof(GlobalConstants), BufferUsage::Dynamic);
	}

	void GlobalSettings::set_override_postprocess(
		float     exposure,
		float     gamma,
		glm::vec4 tint,
		bool      bloom_enabled,
		float     bloom_threshold,
		float     bloom_intensity,
		float     bloom_knee
	) {
		m_has_pp_override = true;
		m_pp_override.pp_exposure = exposure;
		m_pp_override.pp_gamma = gamma;
		m_pp_override.pp_tint = tint;
		m_pp_override.pp_bloom_enabled = bloom_enabled;
		m_pp_override.pp_bloom_threshold = bloom_threshold;
		m_pp_override.pp_bloom_intensity = bloom_intensity;
		m_pp_override.pp_bloom_knee = bloom_knee;
	}

	void GlobalSettings::flush() {
		m_data.projview = projview;
		m_data.prev_projview = prev_projview;
		for (int i = 0; i < 4; ++i) {
			m_data.sun_projview[i] = sun_projview[i];
		}
		m_data.csm_splits = csm_splits;
		m_data.sun_direction = glm::vec4(sun_direction, 0.0f);
		m_data.sun_intensity = sun_color * sun_intensity;
		m_data.cam_position = glm::vec4(cam_position, 0.0f);
		m_data.taa_enabled = (float)taa_enabled;
		m_data.taa_blend = taa_blend;

		if (m_has_pp_override) {
			m_data.pp_exposure = m_pp_override.pp_exposure;
			m_data.pp_gamma = m_pp_override.pp_gamma;
			m_data.pp_tint = m_pp_override.pp_tint;
			m_data.pp_bloom_enabled = (float)m_pp_override.pp_bloom_enabled;
			m_data.pp_bloom_threshold = m_pp_override.pp_bloom_threshold;
			m_data.pp_bloom_intensity = m_pp_override.pp_bloom_intensity;
			m_data.pp_bloom_knee = m_pp_override.pp_bloom_knee;
		}
		else {
			m_data.pp_exposure = pp_exposure;
			m_data.pp_gamma = pp_gamma;
			m_data.pp_tint = pp_tint;
			m_data.pp_bloom_enabled = (float)pp_bloom_enabled;
			m_data.pp_bloom_threshold = pp_bloom_threshold;
			m_data.pp_bloom_intensity = pp_bloom_intensity;
			m_data.pp_bloom_knee = pp_bloom_knee;
		}

		// TAA upgrade fields
		m_data.taa_variance_scale = taa_variance_scale;
		m_data.taa_clip_gamma = taa_clip_gamma;
		m_data.taa_jitter_u = taa_jitter_uv.x;
		m_data.taa_jitter_v = taa_jitter_uv.y;
		m_data.taa_sharpen_enabled = (float)taa_sharpen_enabled;
		m_data.taa_sharpen_strength = taa_sharpen_strength;

		m_data.sun_ambient = sun_ambient_color * sun_ambient_intensity;

		// Ambient Occlusion fields
		m_data.ao_enabled = (float)ao_enabled;
		m_data.ao_type = (float)ao_type;
		m_data.ao_radius = ao_radius;
		m_data.ao_intensity = ao_intensity;
		m_data.ao_power = ao_power;
		m_data.ao_bias = ao_bias;
		m_data.ao_blur_enabled = (float)ao_blur_enabled;
		m_data.ao_blur_strength = ao_blur_strength;

		// SSR is only valid for deferred rendering.
		bool const ssr_runtime_enabled = (render_mode == RenderMode::Deferred) && ssr_enabled;
		m_data.ssr_enabled = ssr_runtime_enabled ? 1.0f : 0.0f;
		m_data.ssr_intensity = ssr_intensity;
		m_data.ssr_max_distance = ssr_max_distance;
		m_data.ssr_thickness = ssr_thickness;
		m_data.ssr_stride = ssr_stride;
		m_data.ssr_max_steps = ssr_max_steps;
		m_data.ssr_jitter_strength = ssr_jitter_strength;

		m_global_buffer->write(&m_data);
	}

	void GlobalSettings::reset_override() {
		// Reset override flag so next frame we start fresh (unless set_override is called again)
		m_has_pp_override = false;
	}

	void GlobalSettings::bind() {
		m_global_buffer->bind();
	}

	void GlobalSettings::unbind() {
		m_global_buffer->unbind();
	}

	uint32_t GlobalSettings::get_binding() const {
		return m_global_buffer->get_binding();
	}

}
