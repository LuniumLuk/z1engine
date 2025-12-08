#include "pch.h"
#include "render/rhi/opengl_context.h"
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
	}

	void OpenGLContext::swap_buffers() {
		PROFILE_FUNCTION();
		glfwSwapBuffers(m_window);
	}

}
