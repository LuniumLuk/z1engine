#include "pch.h"
#include "core/window.h"
#include "core/application.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "event/application_event.h"
#include "glfw/glfw3.h"

namespace z1 {

	static bool s_is_glfw_initialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		CORE_ERROR("glfw error ({0}): {1}", error, description);
	}

	Window::~Window() {
		shutdown();
	}

	void Window::init(Window::Config const& config) {
		PROFILE_FUNCTION();
		m_data.title = config.title;
		m_data.width = config.width;
		m_data.height = config.height;

		CORE_DEBUG("creating window {0} ({1}, {2})", config.title, config.width, config.height);

		if (!s_is_glfw_initialized) {
			{
				PROFILE_SCOPE("glfwInit");
				int success = glfwInit();
				CORE_ASSERT(success, "could not initialize GLFW!");
				glfwSetErrorCallback(GLFWErrorCallback);

				s_is_glfw_initialized = true;
			}
		}

		{
			PROFILE_SCOPE("glfwCreateWindow");
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#ifdef PLATFORM_MACOS
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
			m_window = glfwCreateWindow((int)config.width, (int)config.height, config.title.c_str(), nullptr, nullptr);
			glfwSetWindowUserPointer(m_window, &m_data);

			// Track framebuffer pixel size (not window coords) for correct viewport on HiDPI
			{
				int fb_width, fb_height;
				glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
				m_data.width = (uint32_t)fb_width;
				m_data.height = (uint32_t)fb_height;
			}
		}

		glfwSetFramebufferSizeCallback(m_window,
			[](GLFWwindow* handle, int width, int height) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));
				data->width = (uint32_t)width;
				data->height = (uint32_t)height;

				WindowResizeEvent event((uint32_t)width, (uint32_t)height);
				for (auto const& cb : data->event_callbacks) cb(event);
			});

		glfwSetWindowCloseCallback(m_window,
			[](GLFWwindow* handle) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				WindowCloseEvent event;
				for (auto const& cb : data->event_callbacks) cb(event);
			});

		glfwSetKeyCallback(m_window,
			[](GLFWwindow* handle, int key, int scancode, int action, int mods) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				switch (action) {
				case GLFW_PRESS: {
					KeyPressedEvent event(key, 0);
					for (auto const& cb : data->event_callbacks) cb(event);
					break;
				}
				case GLFW_RELEASE: {
					KeyReleasedEvent event(key, 0);
					for (auto const& cb : data->event_callbacks) cb(event);
					break;
				}
				case GLFW_REPEAT: {
					KeyPressedEvent event(key, 1);
					for (auto const& cb : data->event_callbacks) cb(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_window,
			[](GLFWwindow* handle, uint32_t key) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				KeyTypedEvent event(key);
				for (auto const& cb : data->event_callbacks) cb(event);
			});

		glfwSetMouseButtonCallback(m_window,
			[](GLFWwindow* handle, int button, int action, int mods) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				switch (action) {
				case GLFW_PRESS: {
					MouseButtonPressedEvent event(button);
					for (auto const& cb : data->event_callbacks) cb(event);
					break;
				}
				case GLFW_RELEASE: {
					MouseButtonReleasedEvent event(button);
					for (auto const& cb : data->event_callbacks) cb(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_window,
			[](GLFWwindow* handle, double xoffset, double yoffset) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				MouseScrollEvent event((float)xoffset, (float)yoffset);
				for (auto const& cb : data->event_callbacks) cb(event);
			});

		glfwSetCursorPosCallback(m_window,
			[](GLFWwindow* handle, double xpos, double ypos) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				MouseMovedEvent event((float)xpos, (float)ypos);
				for (auto const& cb : data->event_callbacks) cb(event);
			});

		glfwSetWindowFocusCallback(m_window,
			[](GLFWwindow* handle, int focused) {
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(handle));

				if (focused) {
					WindowFocusEvent event;
					for (auto const& cb : data->event_callbacks) cb(event);
				}
				else {
					WindowLostFocusEvent event;
					for (auto const& cb : data->event_callbacks) cb(event);
				}
			});
	}

	void Window::shutdown() {
		PROFILE_FUNCTION();
		glfwDestroyWindow(m_window);
	}

	void Window::on_update() {
		glfwPollEvents();
	}

	void Window::set_v_sync(bool enabled) {
		if (enabled) {
			glfwSwapInterval(1);
		}
		else {
			glfwSwapInterval(0);
		}
		m_data.v_sync = enabled;
	}

	bool Window::is_v_sync_enabled() const {
		return m_data.v_sync;
	}

	void Window::set_window_title(std::string const& title) {
		glfwSetWindowTitle(m_window, title.c_str());
	}

	void Window::hide_cursor() {
		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		m_data.cursor_hidden = true;
	}

	void Window::show_cursor() {
		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		m_data.cursor_hidden = false;
	}

	void Window::center_cursor() {
		int win_width, win_height;
		glfwGetWindowSize(m_window, &win_width, &win_height);
		glfwSetCursorPos(m_window, win_width / 2.0, win_height / 2.0);
	}

	bool Window::is_cursor_hidden() const {
		return m_data.cursor_hidden;
	}

}
