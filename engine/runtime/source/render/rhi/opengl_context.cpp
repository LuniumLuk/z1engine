#include "pch.h"
#include "render/rhi/opengl_context.h"
#include "render/rhi/opengl_framebuffer.h"
#include "render/pipeline.h"
#include "render/render_pass.h"
#include "core/core.h"
#include "core/window.h"
#include "glad/glad.h"
#include "glfw/glfw3.h"

namespace z1 {

	OpenGLContext::OpenGLContext()
		: m_window{ static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window()) } {
		CORE_ASSERT(m_window, "window handle is null!")
	}

	void OpenGLContext::init() {
		PROFILE_FUNCTION();
		glfwMakeContextCurrent(m_window);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		CORE_ASSERT(status, "failed to initialize Glad!");

		CORE_DEBUG("opengl info:");
		CORE_DEBUG("    vendor: {0}", (char*)glGetString(GL_VENDOR));
		CORE_DEBUG("    renderer: {0}", (char*)glGetString(GL_RENDERER));
		CORE_DEBUG("    version: {0}", (char*)glGetString(GL_VERSION));

		if (glDebugMessageCallback) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}

		GLint val;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
		m_max_image_binding_count = static_cast<uint32_t>(val);
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &val);
		m_max_uniform_buffer_binding_count = static_cast<uint32_t>(val);

		m_free_image_bindings = {};
		for (uint32_t i = m_max_image_binding_count - 1; i != uint32_t(-1); --i) {
			m_free_image_bindings.push(i);
		}

		m_free_uniform_buffer_bindings = {};
		for (uint32_t i = m_max_uniform_buffer_binding_count - 1; i != uint32_t(-1); --i) {
			m_free_uniform_buffer_bindings.push(i);
		}

		m_swapchain_framebuffer = std::make_shared<OpenGLSwapChainFramebuffer>();
		m_current_framebuffer = m_swapchain_framebuffer;
		m_current_pipeline = nullptr;
	}

	void OpenGLContext::swap_buffers() {
		PROFILE_FUNCTION();
		glfwSwapBuffers(m_window);
	}

	void OpenGLContext::bind_framebuffer(std::shared_ptr<Framebuffer> const& framebuffer) {
		m_current_framebuffer = framebuffer;
		if (framebuffer) {
			framebuffer->bind();
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void OpenGLContext::bind_pipeline(std::shared_ptr<Pipeline> const& pipeline) {
		m_current_pipeline = pipeline;
		if (pipeline) {
			pipeline->bind();
		}
	}

	void OpenGLContext::exec_render_pass(std::shared_ptr<RenderPass> const& render_pass) {
		auto& desc = render_pass->desc;

		for (uint32_t i = 0; i < desc.color_attachments.size(); ++i) {
			auto& attachment = desc.color_attachments[i];
			if (attachment.load_op == LoadOp::Clear) {
				glColorMaski(i, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				glClearBufferfv(GL_COLOR, i, &attachment.clear_value[0]);
			}
		}

		if (desc.depth_stencil_attachment.depth_load_op == LoadOp::Clear) {
			glDepthMask(GL_TRUE);
			glClearBufferfv(GL_DEPTH, 0, &desc.depth_stencil_attachment.clear_depth_value);
		}

		if (desc.depth_stencil_attachment.stencil_load_op == LoadOp::Clear) {
			glStencilMask(0xff);
			glClearBufferiv(GL_STENCIL, 0, (GLint*)&desc.depth_stencil_attachment.clear_stencil_value);
		}

		if (desc.dynamic_viewport) {
			set_viewport(0, 0, m_current_framebuffer->get_width(), m_current_framebuffer->get_height());
		}

		if (desc.dynamic_scissor) {
			glDisable(GL_SCISSOR_TEST);
		}

		render_pass->execute(*this);
	}

	void OpenGLContext::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		glViewport(x, y, width, height);
	}

	void OpenGLContext::set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(x, y, width, height);
	}

	void OpenGLContext::push_debug_group(std::string const& name) {
		glPushDebugGroup(
			GL_DEBUG_SOURCE_APPLICATION,
			0,
			-1,
			name.c_str()
		);
	}

	void OpenGLContext::pop_debug_group() {
		glPopDebugGroup();
	}

	void OpenGLContext::blit_attachment(
		std::shared_ptr<Framebuffer> const& src,
		std::shared_ptr<Framebuffer> const& dst,
		uint32_t src_attachment,
		uint32_t dst_attachment,
		uint32_t src_x, uint32_t src_y,
		uint32_t dst_x, uint32_t dst_y,
		uint32_t width, uint32_t height) {

		GLuint src_handle = (GLuint)reinterpret_cast<uintptr_t>(src->get_native_handle());
		GLuint dst_handle = (GLuint)reinterpret_cast<uintptr_t>(dst->get_native_handle());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, src_handle);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + src_attachment);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handle);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + dst_attachment);

		glBlitFramebuffer(
			src_x, src_y, width, height,
			dst_x, dst_y, width, height,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST
		);
	}

	void OpenGLContext::blit_depth_stencil(
		std::shared_ptr<Framebuffer> const& src,
		std::shared_ptr<Framebuffer> const& dst,
		uint32_t src_x, uint32_t src_y,
		uint32_t dst_x, uint32_t dst_y,
		uint32_t width, uint32_t height) {

		GLuint src_handle = (GLuint)reinterpret_cast<uintptr_t>(src->get_native_handle());
		GLuint dst_handle = (GLuint)reinterpret_cast<uintptr_t>(dst->get_native_handle());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, src_handle);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handle);

		glBlitFramebuffer(
			src_x, src_y, width, height,
			dst_x, dst_y, width, height,
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			GL_NEAREST
		);
	}

}
