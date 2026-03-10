#pragma once

#include "core/core.h"
#include "render/resource.h"
#include "render/render_pass.h"
#include "render/shader.h"

namespace z1 {

	enum struct API BlendFactor : int {
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

	enum struct API CullMode : int {
		None = 0,
		Front,
		Back,
		FrontAndBack,
	};

	struct API Pipeline : RenderResource {

		struct Description {
			// depth
			bool depth_test = false;

			// blend
			bool blend = false;
			BlendFactor src_blend_factor = BlendFactor::SrcAlpha;
			BlendFactor dst_blend_factor = BlendFactor::OneMinusSrcAlpha;

			// culling
			CullMode cull_mode = CullMode::Back;

			// shader
			std::shared_ptr<Shader> shader = nullptr;
		};

		Pipeline() : RenderResource(ResourceType::Pipeline) {}

		static std::shared_ptr<Pipeline> build(Description const& description);

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		std::shared_ptr<Shader> m_shader = nullptr;
	};

}
