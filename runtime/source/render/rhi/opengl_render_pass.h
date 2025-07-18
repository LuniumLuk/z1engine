#pragma once

#include "render/render_pass.h"
#include <functional>

namespace z1 {

	struct OpenGLRenderPass : RenderPass {
		OpenGLRenderPass() = default;
		~OpenGLRenderPass() = default;

		void bind(BeginInfo const& info) override;
		void unbind() override;

	private:
		std::shared_ptr<Framebuffer> m_framebuffer = nullptr;
	};

}
