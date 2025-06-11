#pragma once

#include "core/core.h"

namespace z1 {

	struct API GraphicsContext {
		virtual void init() = 0;
		virtual void begin_frame() = 0;
		virtual void end_frame() = 0;
		virtual void swap_buffers() = 0;
		virtual void finish() = 0;

		static std::shared_ptr<GraphicsContext> GraphicsContext::create();

		uint32_t m_max_image_binding_count = 0;
		uint32_t m_max_uniform_buffer_binding_count = 0;
	};

}
