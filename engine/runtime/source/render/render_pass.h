#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {
	struct GraphicsContext;

	enum struct LoadOp : int { Load, Clear, DontCare };
	enum struct StoreOp : int { Store, DontCare };

	struct RenderTargetDesc {
		LoadOp load_op = LoadOp::Load;
		StoreOp store_op = StoreOp::Store;
		glm::vec4 clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	struct DepthStencilDesc {
		LoadOp depth_load_op = LoadOp::Load;
		StoreOp depth_store_op = StoreOp::Store;
		float clear_depth_value = 1.0f;
		LoadOp stencil_load_op = LoadOp::Load;
		StoreOp stencil_store_op = StoreOp::Store;
		uint32_t clear_stencil_value = 0;
	};

	struct API RenderPass {
		struct Description {
			std::vector<RenderTargetDesc> color_attachments;
			DepthStencilDesc depth_stencil_attachment;
			bool dynamic_viewport = true;
			bool dynamic_scissor = true;
		};

		RenderPass(Description const& desc) : desc(desc) {}

		Description desc;
		std::function<void(GraphicsContext&)> execute;

	};

}
