#include "pch.h"
#include "render/render_pass.h"
#include "render/rhi/opengl_render_pass.h"

namespace z1 {

	std::shared_ptr<RenderPass> RenderPass::build() {
		PROFILE_FUNCTION();
		return std::shared_ptr<RenderPass>(new OpenGLRenderPass());
	}

}
