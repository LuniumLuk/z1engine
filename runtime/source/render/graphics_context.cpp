#include "pch.h"
#include "render/graphics_context.h"
#include "render/rhi/opengl_context.h"

namespace z1 {

	std::shared_ptr<GraphicsContext> GraphicsContext::create() {
		return std::shared_ptr<GraphicsContext>(new OpenGLContext());
	}

	void GraphicsContext::update_stats(float dt) {
		m_stats.frame_time = dt * 1000.0f; // ms
		m_stats.fps = 1.0f / dt;

		m_frame_time_history.push_back(dt);
		if (m_frame_time_history.size() > 1000) {
			m_frame_time_history.pop_front();
		}

		std::vector<float> sorted_times(m_frame_time_history.begin(), m_frame_time_history.end());
		std::sort(sorted_times.begin(), sorted_times.end());

		// 1% low means the 1st percentile of FPS, which is the 99th percentile of frame time.
		// So we take the frame time that is greater than 99% of other frame times.
		size_t index_1_percent = (size_t)(sorted_times.size() * 0.99f);
		if (index_1_percent >= sorted_times.size()) index_1_percent = sorted_times.size() - 1;
		m_stats.low_1_percent = 1.0f / sorted_times[index_1_percent];

		size_t index_5_percent = (size_t)(sorted_times.size() * 0.95f);
		if (index_5_percent >= sorted_times.size()) index_5_percent = sorted_times.size() - 1;
		m_stats.low_5_percent = 1.0f / sorted_times[index_5_percent];
	}

	uint32_t GraphicsContext::acquire_image_binding() {
		if (m_free_image_bindings.empty()) {
			CORE_ERROR("no free image binding available!");
			return INVALID_BINDING;
		}

		uint32_t binding = m_free_image_bindings.top();
		m_free_image_bindings.pop();
		return binding;
	}

	void GraphicsContext::release_image_binding(uint32_t binding) {
		if (binding >= m_max_image_binding_count) {
			CORE_ERROR("invalid image binding released: {0}", binding);
			return;
		}

		m_free_image_bindings.push(binding);
	}

	uint32_t GraphicsContext::acquire_uniform_buffer_binding() {
		if (m_free_uniform_buffer_bindings.empty()) {
			CORE_ERROR("no free uniform_buffer binding available!");
			return INVALID_BINDING;
		}

		uint32_t binding = m_free_uniform_buffer_bindings.top();
		m_free_uniform_buffer_bindings.pop();
		return binding;
	}

	void GraphicsContext::release_uniform_buffer_binding(uint32_t binding) {
		if (binding >= m_max_uniform_buffer_binding_count) {
			CORE_ERROR("invalid uniform_buffer binding released: {0}", binding);
			return;
		}

		m_free_uniform_buffer_bindings.push(binding);
	}

}
