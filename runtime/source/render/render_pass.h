#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include <stack>

namespace z1 {
	struct Framebuffer;

	enum struct API BlendFactor {
		Zero = 0,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
	};

	enum struct API CullMode {
		None = 0,
		Front,
		Back,
		FrontAndBack,
	};
	
	// Here a RenderPass is a combination of RenderPass(SubPass) and Pipeline in Vulkan
	// That is, the RenderPass here is a set of rendering states and a shader program that can be bound to a framebuffer.
	struct API RenderPass {

		struct Description {
			// depth
			bool depth_test = false;

			// blend
			bool blend = false;
			BlendFactor src_blend_factor = BlendFactor::SrcAlpha;
			BlendFactor dst_blend_factor = BlendFactor::OneMinusSrcAlpha;

			// culling
			CullMode cull_mode = CullMode::Back;
		};

		static std::shared_ptr<RenderPass> build(Description const& description);

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

		virtual void begin(BeginInfo const& info) = 0;
		virtual void end() = 0;
	};

}
