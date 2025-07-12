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

		m_framebuffer = description.m_framebuffer;
		m_shader = description.m_shader;

		m_dynamic_viewport = description.m_dynamic_viewport;

		m_scissor = description.m_scissor;
		m_dynamic_scissor = description.m_dynamic_scissor;

		m_bind_func = [this, description] {
			if (!description.m_dynamic_viewport) {
				uint32_t w = description.m_viewport_width;
				uint32_t h = description.m_viewport_height;

				if (w == -1) {
					w = m_framebuffer ? m_framebuffer->get_description().m_width : g_runtime_context.m_window->get_width();
				}

				if (h == -1) {
					h = m_framebuffer ? m_framebuffer->get_description().m_height : g_runtime_context.m_window->get_height();
				}

				glViewport(
					description.m_viewport_x,
					description.m_viewport_y,
					w,
					h);
			}

			if (m_scissor && !description.m_dynamic_scissor) {
				uint32_t w = description.m_scissor_width;
				uint32_t h = description.m_scissor_height;

				if (w == -1) {
					w = m_framebuffer ? m_framebuffer->get_description().m_width : g_runtime_context.m_window->get_width();
				}

				if (h == -1) {
					h = m_framebuffer ? m_framebuffer->get_description().m_height : g_runtime_context.m_window->get_height();
				}

				glEnable(GL_SCISSOR_TEST);
				glScissor(
					description.m_scissor_x,
					description.m_scissor_y,
					w,
					h);
			}

			glClearColor(
				description.m_clear_color_value.r,
				description.m_clear_color_value.g,
				description.m_clear_color_value.b,
				description.m_clear_color_value.a);
			glClearDepth(description.m_clear_depth_value);

			uint32_t clear_flag = 0;
			if (description.m_clear_color) {
				clear_flag |= GL_COLOR_BUFFER_BIT;
			}
			if (description.m_clear_depth) {
				clear_flag |= GL_DEPTH_BUFFER_BIT;
			}
			if (clear_flag) {
				glClear(clear_flag);
			}

			if (description.m_depth_test) {
				glEnable(GL_DEPTH_TEST);
			}
			else {
				glDisable(GL_DEPTH_TEST);
			}

			if (description.m_cull_mode == CullMode::None) {
				glDisable(GL_CULL_FACE);
			}
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(cull_mode_to_opengl_type(description.m_cull_mode));
			}

			if (description.m_blend) {
				glEnable(GL_BLEND);
				glBlendFunc(
					blend_factor_to_opengl_type(description.m_src_blend_factor),
					blend_factor_to_opengl_type(description.m_dst_blend_factor));
			}
			else {
				glDisable(GL_BLEND);
			}
		};

		m_unbind_func = [this] {

		};
	}

	OpenGLRenderPass::~OpenGLRenderPass() {

	}

	void OpenGLRenderPass::bind() {
		if (!m_framebuffer) m_framebuffer = g_runtime_context.m_main_framebuffer;

		if (m_framebuffer) {
			m_framebuffer->bind();
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		m_bind_func();
		if (m_shader) m_shader->bind();
	}

	void OpenGLRenderPass::unbind() {
		if (m_shader) m_shader->unbind();
		m_unbind_func();
		if (m_framebuffer) m_framebuffer->unbind();
	}

}
