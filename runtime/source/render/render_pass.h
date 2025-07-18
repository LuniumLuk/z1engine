#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include <stack>

namespace z1 {
	struct Framebuffer;

	struct API RenderPass {
		struct BeginInfo {
			// clear
			bool clear_color = false;
			bool clear_depth = false;
			glm::vec4 clear_color_value = { 0.0f, 0.0f, 0.0f, 0.0f };
			float clear_depth_value = 1.0f;

			// viewport
			bool dynamic_viewport = false;
			uint32_t viewport_x = 0;
			uint32_t viewport_y = 0;
			uint32_t viewport_width = UINT32_MAX;
			uint32_t viewport_height = UINT32_MAX;

			// scissor
			bool scissor = false;
			bool dynamic_scissor = false;
			uint32_t scissor_x = 0;
			uint32_t scissor_y = 0;
			uint32_t scissor_width = UINT32_MAX;
			uint32_t scissor_height = UINT32_MAX;

			// framebuffer
			std::shared_ptr<Framebuffer> framebuffer = nullptr;
		};

		RenderPass() = default;
		static std::shared_ptr<RenderPass> build();

		virtual void bind(BeginInfo const& info) = 0;
		virtual void unbind() = 0;
	};

}
