#include "pch.h"
#include "core/core.h"
#include "core/application.h"
#include "core/window.h"
#include "core/timer.h"
#include "core/input.h"
#include "core/layer_stack.h"
#include "3rdparty/imgui_layer.h"
#include "event/application_event.h"
#include "render/graphics_context.h"

#include "pybind11/embed.h"
namespace py = pybind11;

extern void ForceLinkPythonEngine();

namespace z1 {

	Application::Application() {
		PROFILE_BEGIN_SESSION("application init", "profile-init.json");
		PROFILE_FUNCTION();

		g_runtime_context.init();
		g_runtime_context.m_window->add_event_callback(BIND_EVENT_FN(Application::on_event));
	}

	Application::~Application() {
		g_runtime_context.shutdown();
		PROFILE_END_SESSION();
	}

	void Application::run() {
		PROFILE_END_SESSION();
		PROFILE_BEGIN_SESSION("application run", "profile-run.json");
		push_overlay(std::static_pointer_cast<Layer>(g_runtime_context.m_imgui_layer));


		// temporary test space for python script runner
		ForceLinkPythonEngine();

		// 1. Initialize PyConfig
		PyConfig config;
		PyConfig_InitIsolatedConfig(&config); // Isolated means ignore environment variables

		// 2. Set the Python Home (the directory containing python314.zip or Lib/)
		// This replaces Py_SetPythonHome
		PyStatus status = PyConfig_SetString(&config, &config.home, L"./pyenv");
		if (PyStatus_Exception(status)) {
			PyConfig_Clear(&config);
			CORE_ASSERT(false, "Failed to set Python Home");
		}

		// 3. Apply the configuration and initialize the interpreter
		status = Py_InitializeFromConfig(&config);
		PyConfig_Clear(&config); // Done with config, clear memory
		if (PyStatus_Exception(status)) {
			CORE_ASSERT(false, "Failed to initialize Python interpreter");
		}

		// 4. Now that Python is started, pybind11 can wrap it
		// We don't use scoped_interpreter here because we initialized manually
		try {
			py::exec(R"(
			import sys
			import os
			print(f"Python Home: {sys.prefix}")
			print(f"Searching in: {sys.path}")

			import Engine
			Engine.log("Python Path verified!")
		)");
		}
		catch (py::error_already_set& e) {
			CORE_ERROR("Python Error: {0}", e.what());
		}

		// 5. Cleanup manually at the end of the program
		Py_Finalize();

		g_runtime_context.m_timer->update();
		while (!m_should_exit) {
			g_runtime_context.m_graphics_context->begin_frame();
			if (!m_minimized) {
				{
					PROFILE_SCOPE("update layer stacks");
					for (auto it = g_runtime_context.m_layer_stack->end(); it != g_runtime_context.m_layer_stack->begin();) {
						--it;
						(*it)->on_update(g_runtime_context.m_timer->get_delta_time());
						if (g_runtime_context.m_timer->should_fixed_update())
							(*it)->on_fixed_update();
					}
				}

				{
					PROFILE_SCOPE("ImGuiRender");
					g_runtime_context.m_imgui_layer->begin();
					for (auto it = g_runtime_context.m_layer_stack->end(); it != g_runtime_context.m_layer_stack->begin();)
						(*--it)->on_imgui_render();
					g_runtime_context.m_imgui_layer->end();
				}
			}

			g_runtime_context.m_input_system->reset();
			g_runtime_context.m_window->on_update();
			g_runtime_context.m_graphics_context->end_frame();
			g_runtime_context.m_graphics_context->swap_buffers();
			g_runtime_context.m_timer->update();
		}
		g_runtime_context.m_graphics_context->finish();
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
