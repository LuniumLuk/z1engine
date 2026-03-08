#include "pch.h"

#include "render/global.h"
#include "render/buffer.h"

namespace z1 {

	GlobalSettings::GlobalSettings() {
		m_global_buffer = UniformBuffer::create(nullptr, sizeof(GlobalConstants), BufferUsage::Dynamic);
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
		m_data.pp_exposure = pp_exposure;
		m_data.pp_gamma = pp_gamma;
		m_data.pp_tint = pp_tint;
		m_data.sun_ambient = sun_ambient_color * sun_ambient_intensity;

		m_global_buffer->write(&m_data);
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
