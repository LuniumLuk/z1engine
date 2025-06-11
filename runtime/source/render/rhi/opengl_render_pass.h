#pragma once

#include "render/render_pass.h"
#include <functional>

namespace z1 {

	struct OpenGLRenderPass : RenderPass {

		OpenGLRenderPass(Description const& description);
		~OpenGLRenderPass();

		void bind() override;
		void unbind() override;

	private:
		std::function<void()> m_bind_func;
		std::function<void()> m_unbind_func;

		bool m_dynamic_viewport;
		bool m_scissor;
		bool m_dynamic_scissor;
	};

}
