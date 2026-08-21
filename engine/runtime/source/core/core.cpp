#include "pch.h"
#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/timer.h"
#include "core/log.h"
#include "core/io.h"
#include "core/input.h"
#include "scene/scene.h"
#include "scene/script_system.h"
#include "render/global.h"
#include "render/resource.h"
#include "render/graphics_context.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"
#include "render/renderer/renderer_deferred.h"
#include "asset/asset_manager.h"
#include "3rdparty/imgui_layer.h"
#include "python/python_layer.h"

namespace z1 {

	RuntimeContext g_runtime_context;

	void RuntimeContext::init() {
		m_timer = std::make_shared<Timer>();
		init_logger();
		m_file_system = std::make_shared<FileSystem>();

		bool const no_window = g_args.get<bool>("no-window", false);
		bool const no_graphics = g_args.get<bool>("no-graphics", false);

		if (!no_window) {
			m_window = std::make_shared<Window>();
			auto conf = Window::Config{};
			conf.title = g_args.get<std::string>("title", "z1 engine");
			conf.width = g_args.get<uint32_t>("width", 1280);
			conf.height = g_args.get<uint32_t>("height", 720);
			m_window->init(conf);

			m_input_system = std::make_shared<InputSystem>(m_window);
		}

		if (!no_graphics) {
			m_graphics_context = GraphicsContext::create();
			m_graphics_context->init();
		}

		m_asset_manager = std::make_shared<AssetManager>();

		if (!no_window && !no_graphics) {
			m_imgui_layer = std::make_shared<ImGuiLayer>();
		}
		m_python_layer = std::make_shared<PythonLayer>();
		m_layer_stack = std::make_shared<LayerStack>();

		//m_renderer_2d = std::make_shared<Renderer2D>();
		if (!no_graphics) {
			m_renderer_forward = std::make_shared<RendererForward>();
			m_renderer_deferred = std::make_shared<RendererDeferred>();
		}

		m_global = std::make_shared<GlobalSettings>();
	}

	void RuntimeContext::init_logger() {
		if (!m_logger) {
			m_logger = std::make_shared<Logger>();
		}
	}

	void RuntimeContext::shutdown() {
		if (m_window) {
			m_window->clear_event_callbacks();
		}

		// layers are detached and destroyed first: their teardown runs while the engine services
		// (window, renderers, global settings, scene) are still alive
		m_layer_stack.reset();

		m_global.reset();
		m_renderer_deferred.reset();
		m_renderer_forward.reset();
		//m_renderer_2d.reset();

		if (m_scene)
			m_scene.reset();
		m_python_layer.reset();
		m_imgui_layer.reset();

		m_asset_manager.reset();

		m_graphics_context.reset();

		m_input_system.reset();

		m_window.reset();

		m_file_system.reset();
		m_logger.reset();
		m_timer.reset();
	}

}
