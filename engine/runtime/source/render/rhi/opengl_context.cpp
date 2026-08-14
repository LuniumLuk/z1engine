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

	void glCheckError_(const char *file, int line) {
		GLenum code;
		while ((code = glGetError()) != GL_NO_ERROR) {
			std::string error;
			switch (code) {
			case GL_INVALID_ENUM:                   error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                  error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:              error = "INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:                 error = "STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:                error = "STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:                  error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION"; break;
			}
			CORE_ERROR("OpenGL Error: {0} | {1} ({2})", error, file, line);
		}
	}

	static void APIENTRY gl_debug_message_callback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		GLchar const* message,
		void const* userParam) {
		(void)source;
		(void)type;
		(void)id;
		(void)length;
		(void)userParam;

		// Route GL debug messages to the matching log level by severity.
		// Informational notifications (debug-group markers, driver buffer
		// allocation hints, etc.) go to trace level, which is hidden by the
		// default info-level logger, so they no longer spam the log as warns.
		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			CORE_ERROR("OpenGL: {0}", message);
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			CORE_WARN("OpenGL: {0}", message);
			break;
		case GL_DEBUG_SEVERITY_LOW:
			CORE_DEBUG("OpenGL: {0}", message);
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
		default:
			CORE_TRACE("OpenGL: {0}", message);
			break;
		}
	}

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
			glDebugMessageCallback(gl_debug_message_callback, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			DEBUG_RUN(glCheckError());
#ifdef DEBUG
			// Synchronous debug output adds overhead; enable only in debug builds
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			DEBUG_RUN(glCheckError());
#endif
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
			DEBUG_RUN(glCheckError());
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
				DEBUG_RUN(glCheckError());
				glClearBufferfv(GL_COLOR, i, &attachment.clear_value[0]);
				DEBUG_RUN(glCheckError());
			}
		}

		if (desc.depth_stencil_attachment.depth_load_op == LoadOp::Clear) {
			glDepthMask(GL_TRUE);
			DEBUG_RUN(glCheckError());
			glClearBufferfv(GL_DEPTH, 0, &desc.depth_stencil_attachment.clear_depth_value);
			DEBUG_RUN(glCheckError());
		}

		if (desc.depth_stencil_attachment.stencil_load_op == LoadOp::Clear) {
			glStencilMask(0xff);
			DEBUG_RUN(glCheckError());
			glClearBufferiv(GL_STENCIL, 0, (GLint*)&desc.depth_stencil_attachment.clear_stencil_value);
			DEBUG_RUN(glCheckError());
		}

		if (desc.dynamic_viewport) {
			set_viewport(0, 0, m_current_framebuffer->get_width(), m_current_framebuffer->get_height());
		}

		if (desc.dynamic_scissor) {
			glDisable(GL_SCISSOR_TEST);
			DEBUG_RUN(glCheckError());
		}

		render_pass->execute(*this);
	}

	void OpenGLContext::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		glViewport(x, y, width, height);
		DEBUG_RUN(glCheckError());
	}

	void OpenGLContext::set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		glEnable(GL_SCISSOR_TEST);
		DEBUG_RUN(glCheckError());
		glScissor(x, y, width, height);
		DEBUG_RUN(glCheckError());
	}

	void OpenGLContext::push_debug_group(std::string const& name) {
#ifdef PLATFORM_MACOS
		(void)name; // glPushDebugGroup requires GL 4.3+ (not available on macOS 4.1)
#else
		glPushDebugGroup(
			GL_DEBUG_SOURCE_APPLICATION,
			0,
			-1,
			name.c_str()
		);
#endif
	}

	void OpenGLContext::pop_debug_group() {
#ifndef PLATFORM_MACOS
		glPopDebugGroup();
#endif
	}

	void OpenGLContext::blit_attachment(
		std::shared_ptr<Framebuffer> const& src,
		std::shared_ptr<Framebuffer> const& dst,
		uint32_t src_attachment,
		uint32_t dst_attachment,
		uint32_t src_x, uint32_t src_y,
		uint32_t dst_x, uint32_t dst_y,
		uint32_t width, uint32_t height) {

		DEBUG_CHECK(src->get_width() == dst->get_width() && src->get_height() == dst->get_height(), "blit_attachment requires src and dst to have the same dimensions!");
		uint32_t w = width;
		uint32_t h = height;
		if (w == NUM_MAX) w = src->get_width();
		if (h == NUM_MAX) h = src->get_height();

		GLuint src_handle = (GLuint)reinterpret_cast<uintptr_t>(src->get_native_handle());
		GLuint dst_handle = (GLuint)reinterpret_cast<uintptr_t>(dst->get_native_handle());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, src_handle);
		DEBUG_RUN(glCheckError());
		glReadBuffer(GL_COLOR_ATTACHMENT0 + src_attachment);
		DEBUG_RUN(glCheckError());

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handle);
		DEBUG_RUN(glCheckError());
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + dst_attachment);
		DEBUG_RUN(glCheckError());

		glColorMaski(dst_attachment, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		DEBUG_RUN(glCheckError());
		glBlitFramebuffer(
			src_x, src_y, w, h,
			dst_x, dst_y, w, h,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST
		);
		DEBUG_RUN(glCheckError());
	}

	void OpenGLContext::blit_depth_stencil(
		std::shared_ptr<Framebuffer> const& src,
		std::shared_ptr<Framebuffer> const& dst,
		uint32_t src_x, uint32_t src_y,
		uint32_t dst_x, uint32_t dst_y,
		uint32_t width, uint32_t height) {

		DEBUG_CHECK(src->get_width() == dst->get_width() && src->get_height() == dst->get_height(), "blit_attachment requires src and dst to have the same dimensions!");
		uint32_t w = width;
		uint32_t h = height;
		if (w == NUM_MAX) w = src->get_width();
		if (h == NUM_MAX) h = src->get_height();

		GLuint src_handle = (GLuint)reinterpret_cast<uintptr_t>(src->get_native_handle());
		GLuint dst_handle = (GLuint)reinterpret_cast<uintptr_t>(dst->get_native_handle());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, src_handle);
		DEBUG_RUN(glCheckError());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handle);
		DEBUG_RUN(glCheckError());

		glDepthMask(GL_TRUE);
		DEBUG_RUN(glCheckError());
		glStencilMask(0xff);
		DEBUG_RUN(glCheckError());
		glBlitFramebuffer(
			src_x, src_y, w, h,
			dst_x, dst_y, w, h,
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			GL_NEAREST
		);
		DEBUG_RUN(glCheckError());
	}

}
