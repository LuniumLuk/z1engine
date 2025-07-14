#include "pch.h"
#include "core/window.h"
#include "render/rhi/opengl_render_pass.h"
#include "render/framebuffer.h"
#include "render/shader.h"
#include "glad/glad.h"

namespace z1 {

	static GLenum blend_factor_to_opengl_type(BlendFactor factor) {
		switch (factor) {
		case BlendFactor::Zero: return GL_ZERO;
		case BlendFactor::One: return GL_ONE;
		case BlendFactor::SrcColor: return GL_SRC_COLOR;
		case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DstColor: return GL_DST_COLOR;
		case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha: return GL_DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
		}
		return GL_ZERO;
	}

	static GLenum cull_mode_to_opengl_type(CullMode mode) {
		switch (mode) {
		case CullMode::Front: return GL_FRONT;
		case CullMode::Back: return GL_BACK;
		case CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
		}
		return GL_NONE;
	}

	OpenGLRenderPass::OpenGLRenderPass(Description const& description) {

		m_shader = description.shader;

		m_begin_func = [this, description] {
			if (description.depth_test) {
				glEnable(GL_DEPTH_TEST);
			}
			else {
				glDisable(GL_DEPTH_TEST);
			}

			if (description.cull_mode == CullMode::None) {
				glDisable(GL_CULL_FACE);
			}
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(cull_mode_to_opengl_type(description.cull_mode));
			}

			if (description.blend) {
				glEnable(GL_BLEND);
				glBlendFunc(
					blend_factor_to_opengl_type(description.src_blend_factor),
					blend_factor_to_opengl_type(description.dst_blend_factor));
			}
			else {
				glDisable(GL_BLEND);
			}
		};

		m_end_func = [this] {

		};
	}

	OpenGLRenderPass::~OpenGLRenderPass() {

	}

	void OpenGLRenderPass::begin(BeginInfo const& info) {
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

		m_begin_func();
		if (m_shader) m_shader->bind();

		m_framebuffer = info.framebuffer;
	}

	void OpenGLRenderPass::end() {
		if (m_shader) m_shader->unbind();
		m_end_func();

		if (m_framebuffer) {
			m_framebuffer->unbind();
			m_framebuffer = nullptr;
		}
	}

}
