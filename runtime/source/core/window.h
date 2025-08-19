#pragma once

#include "core/core.h"
#include "event/event.h"
#include <string>
#include <functional>

struct GLFWwindow;

namespace z1 {

	struct API Window {

		struct Config {
			std::string title = "z1 engine";
			uint32_t width = 1280;
			uint32_t height = 720;
		};

		using EventCallbackFn = std::function<void(Event&)>;

		struct WindowData {
			std::string title;
			uint32_t width = 0;
			uint32_t height = 0;
			bool v_sync = true;
			std::vector<EventCallbackFn> event_callbacks;
		};

		Window() = default;
		~Window();

		void init(Config const& config);
		void on_update();

		uint32_t get_width() const { return m_data.width; }
		uint32_t get_height() const { return m_data.height; }

		void add_event_callback(EventCallbackFn const& callback) { m_data.event_callbacks.push_back(callback); }
		void clear_event_callbacks() { m_data.event_callbacks.clear(); }
		void set_v_sync(bool enabled);
		bool is_v_sync_enabled() const;

		void set_window_title(std::string const& title);

		void* get_native_window() const { return m_window; }

	private:
		void shutdown();

		GLFWwindow* m_window = nullptr;
		WindowData m_data = {};
	};

}
