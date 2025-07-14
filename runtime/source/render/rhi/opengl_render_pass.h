#pragma once

#include "render/render_pass.h"
#include <functional>

namespace z1 {

	struct OpenGLRenderPass : RenderPass {
		OpenGLRenderPass(Description const& description);
		~OpenGLRenderPass();

		void begin(BeginInfo const& info) override;
		void end() override;

	private:
		std::function<void()> m_begin_func;
		std::function<void()> m_end_func;

		std::shared_ptr<Framebuffer> m_framebuffer = nullptr;
	};

}
