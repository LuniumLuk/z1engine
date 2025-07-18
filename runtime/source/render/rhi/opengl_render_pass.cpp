#include "pch.h"
#include "render/rhi/opengl_render_pass.h"
#include "render/framebuffer.h"
#include "glad/glad.h"

namespace z1 {

	void OpenGLRenderPass::bind(BeginInfo const& info) {
		if (info.framebuffer) {
			info.framebuffer->bind();
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (!info.dynamic_viewport) {
			uint32_t w = info.viewport_width;
			uint32_t h = info.viewport_height;

			if (w == -1) {
				w = Framebuffer::get_width(info.framebuffer);
			}

			if (h == -1) {
				h = Framebuffer::get_height(info.framebuffer);
			}

			glViewport(
				info.viewport_x,
				info.viewport_y,
				w,
				h);
		}

		if (info.scissor && !info.dynamic_scissor) {
			uint32_t w = info.scissor_width;
			uint32_t h = info.scissor_height;

			if (w == -1) {
				w = Framebuffer::get_width(info.framebuffer);
			}

			if (h == -1) {
				h = Framebuffer::get_height(info.framebuffer);
			}

			glEnable(GL_SCISSOR_TEST);
			glScissor(
				info.scissor_x,
				info.scissor_y,
				w,
				h);
		}

		glClearColor(
			info.clear_color_value.r,
			info.clear_color_value.g,
			info.clear_color_value.b,
			info.clear_color_value.a);
		glClearDepth(info.clear_depth_value);

		uint32_t clear_flag = 0;
		if (info.clear_color) {
			clear_flag |= GL_COLOR_BUFFER_BIT;
		}
		if (info.clear_depth) {
			clear_flag |= GL_DEPTH_BUFFER_BIT;
		}
		if (clear_flag) {
			glClear(clear_flag);
		}

		m_framebuffer = info.framebuffer;
	}

	void OpenGLRenderPass::unbind() {
		if (m_framebuffer) {
			m_framebuffer->unbind();
			m_framebuffer = nullptr;
		}
	}

}
