#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include <stack>

namespace z1 {
	struct Framebuffer;
	struct Shader;

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

	struct API RenderPass {

		struct Description {
			// clear
			bool m_clear_color = false;
			bool m_clear_depth = false;
			glm::vec4 m_clear_color_value = { 0.0f, 0.0f, 0.0f, 0.0f };
			float m_clear_depth_value = 1.0f;

			// depth
			bool m_depth_test = false;

			// blend
			bool m_blend = false;
			BlendFactor m_src_blend_factor = BlendFactor::SrcAlpha;
			BlendFactor m_dst_blend_factor = BlendFactor::OneMinusSrcAlpha;

			// culling
			CullMode m_cull_mode = CullMode::Back;

			// viewport
			bool m_dynamic_viewport = false;
			uint32_t m_viewport_x = 0;
			uint32_t m_viewport_y = 0;
			uint32_t m_viewport_width = UINT32_MAX;
			uint32_t m_viewport_height = UINT32_MAX;

			// scissor
			bool m_scissor = false;
			bool m_dynamic_scissor = false;
			uint32_t m_scissor_x = 0;
			uint32_t m_scissor_y = 0;
			uint32_t m_scissor_width = UINT32_MAX;
			uint32_t m_scissor_height = UINT32_MAX;

			// framebuffer
			std::shared_ptr<Framebuffer> m_framebuffer;

			// shader
			std::shared_ptr<Shader> m_shader;
		};

		static std::shared_ptr<RenderPass> build(Description const& description);

		virtual void bind() = 0;
		virtual void unbind() = 0;

		std::shared_ptr<Framebuffer> m_framebuffer;
		std::shared_ptr<Shader> m_shader;
	};

}
