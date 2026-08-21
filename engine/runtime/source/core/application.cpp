#include "pch.h"
#include "core/core.h"
#include "core/application.h"
#include "core/window.h"
#include "core/timer.h"
#include "core/input.h"
#include "core/layer_stack.h"
#include "3rdparty/imgui_layer.h"
#include "python/python_layer.h"
#include "event/application_event.h"
#include "render/graphics_context.h"
#include "render/global.h"
#include "util/prober.h"

namespace z1 {

	Application::Application() {
		PROFILE_BEGIN_SESSION("application init", "profile-init.json");
		PROFILE_FUNCTION();

		g_runtime_context.init();
		if (g_runtime_context.m_window) {
			g_runtime_context.m_window->add_event_callback(BIND_EVENT_FN(Application::on_event));
		}
	}

	Application::~Application() {
		g_runtime_context.shutdown();
		PROFILE_END_SESSION();
	}

	void Application::run() {
		PROFILE_END_SESSION();
		PROFILE_BEGIN_SESSION("application run", "profile-run.json");
		push_overlay(std::static_pointer_cast<Layer>(g_runtime_context.m_imgui_layer));
		push_overlay(std::static_pointer_cast<Layer>(g_runtime_context.m_python_layer));

		g_runtime_context.m_timer->update();
		PROBE_CONFIGURE();
		while (!m_should_exit) {
			PROBE_FRAME_BEGIN();
			g_runtime_context.m_graphics_context->update_stats(g_runtime_context.m_timer->get_delta_time());
			g_runtime_context.m_graphics_context->m_stats.reset();
			g_runtime_context.m_graphics_context->begin_frame();

			// ImGui asserts when the monitor list is empty (display asleep), so UI work is skipped
			// while no display is present; rendering resumes as soon as one is available again
			bool const can_render = !m_minimized && g_runtime_context.m_window->is_display_available();
			if (can_render) {
				{
					PROBE_SCOPE("update");
					PROFILE_SCOPE("update layer stacks");
					for (auto it = g_runtime_context.m_layer_stack->end(); it != g_runtime_context.m_layer_stack->begin();) {
						--it;
						(*it)->on_update(g_runtime_context.m_timer->get_delta_time());
						if (g_runtime_context.m_timer->should_fixed_update())
							(*it)->on_fixed_update();
					}
				}

				{
					PROBE_SCOPE("imgui");
					PROFILE_SCOPE("ImGuiRender");
					g_runtime_context.m_imgui_layer->begin();
					for (auto it = g_runtime_context.m_layer_stack->end(); it != g_runtime_context.m_layer_stack->begin();)
						(*--it)->on_imgui_render();
					g_runtime_context.m_imgui_layer->end();
				}
			}

			g_runtime_context.m_input_system->reset();
			{
				PROBE_SCOPE("window_update");
				g_runtime_context.m_window->on_update();
			}
			g_runtime_context.m_graphics_context->end_frame();
			{
				PROBE_SCOPE("swap");
				g_runtime_context.m_graphics_context->swap_buffers();
			}
			g_runtime_context.m_timer->update();
			g_runtime_context.m_global->reset_override();
			PROBE_VALUE("draws", g_runtime_context.m_graphics_context->m_stats.draw_calls);
			PROBE_FRAME_END();
		}
		g_runtime_context.m_graphics_context->finish();
		PROBE_PRINT_SUMMARY();
		PROFILE_END_SESSION();
		PROFILE_BEGIN_SESSION("application shutdown", "profile-shutdown.json");
	}

	void Application::terminate() {
		m_should_exit = true;
	}

	void Application::on_event(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::on_window_close));
		dispatcher.dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::on_window_resize));

		for (auto it = g_runtime_context.m_layer_stack->end(); it != g_runtime_context.m_layer_stack->begin();) {
			(*--it)->on_event(event);
			if (event.is_handled())
				break;
		}
	}

	bool Application::on_window_close(WindowCloseEvent& event) {
		terminate();
		return false;
	}

	bool Application::on_window_resize(WindowResizeEvent& event) {
		CORE_DEBUG("resize {0} {1}", event.get_width(), event.get_height());
		m_minimized = (event.get_width() == 0 || event.get_height() == 0);

		return false;
	}

	void Application::push_layer(std::shared_ptr<Layer> const& layer) {
		layer->m_attached_application = this;
		g_runtime_context.m_layer_stack->push_layer(layer);
	}

	void Application::push_overlay(std::shared_ptr<Layer> const& overlay) {
		overlay->m_attached_application = this;
		g_runtime_context.m_layer_stack->push_overlay(overlay);
	}

}
