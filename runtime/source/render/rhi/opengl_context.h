#pragma once

#include "render/graphics_context.h"

struct GLFWwindow;

namespace z1 {

	struct OpenGLContext : GraphicsContext {
		OpenGLContext();

		void init() override;
		void begin_frame() override {}
		void end_frame() override {}
		void swap_buffers() override;
		void finish() override {}

		void bind_framebuffer(std::shared_ptr<Framebuffer> const& framebuffer) override;
		void bind_pipeline(std::shared_ptr<Pipeline> const& pipeline) override;

		void exec_render_pass(std::shared_ptr<RenderPass> const& render_pass) override;

		void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

	private:
		GLFWwindow*  m_window;
	};

}
