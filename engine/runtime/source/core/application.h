#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "event/application_event.h"

namespace z1 {

	struct API Application {
		Application();
		virtual ~Application();

		virtual void init() {};
		void run();
		void terminate();

		void on_event(Event& event);
		bool on_window_close(WindowCloseEvent& event);
		bool on_window_resize(WindowResizeEvent& event);

		// layers are executed before all the other overlays
		// layers are executed in the order they were pushed
		void push_layer(std::shared_ptr<Layer> const& layer);
		// overlays are executed after all the other layers
		// overlays are executed in the order they were pushed
		void push_overlay(std::shared_ptr<Layer> const& overlay);

		bool m_should_exit = false;
		bool m_minimized = false;
	};

}
