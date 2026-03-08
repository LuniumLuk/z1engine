#pragma once

#include "core/core.h"
#include "core/window.h"
#include "event/mouse_event.h"

namespace z1 {

	struct API InputSystem {
		InputSystem(std::shared_ptr<Window> const& window);

		bool is_key_pressed(int keycode) const;
		bool is_mouse_button_pressed(int button) const;
		std::pair<float, float> get_mouse_pos() const;
		float get_scroll_delta() const;

	private:
		friend struct Application;
		void reset();
		void on_event(Event& event);
		bool on_mouse_scrolled(MouseScrollEvent& event);
		float m_scroll_delta = 0.0f;
	};

}
