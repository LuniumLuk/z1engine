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
#include "render/render_graph.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"
#include "3rdparty/imgui_layer.h"
#include "python/python_layer.h"

namespace z1 {

	RuntimeContext g_runtime_context;

	void RuntimeContext::init() {
		m_timer = std::make_shared<Timer>();
		init_logger();
		m_file_system = std::make_shared<FileSystem>();

		m_window = std::make_shared<Window>();
		m_window->init(Window::Config());

		m_input_system = std::make_shared<InputSystem>(m_window);

		m_graphics_context = GraphicsContext::create();
		m_graphics_context->init();

		m_asset_manager = std::make_shared<AssetManager>();

		m_imgui_layer = std::make_shared<ImGuiLayer>();
		m_python_layer = std::make_shared<PythonLayer>();
		m_layer_stack = std::make_shared<LayerStack>();

		m_renderer_2d = std::make_shared<Renderer2D>();
		m_renderer_forward = std::make_shared<RendererForward>();

		m_global = std::make_shared<GlobalSettings>();
	}

	void RuntimeContext::init_logger() {
		if (!m_logger) {
			m_logger = std::make_shared<Logger>();
		}
	}

	void RuntimeContext::shutdown() {
		m_window->clear_event_callbacks();

		m_global.reset();
		m_renderer_forward.reset();
		m_renderer_2d.reset();

		RenderGraph::clear_cache();

		m_layer_stack.reset();
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
