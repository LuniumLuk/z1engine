#include "pch.h"
#include "render/graphics_context.h"
#include "render/rhi/opengl_context.h"

namespace z1 {

	std::shared_ptr<GraphicsContext> GraphicsContext::create() {
		return std::shared_ptr<GraphicsContext>(new OpenGLContext());
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