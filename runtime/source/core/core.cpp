#include "pch.h"
#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/timer.h"
#include "core/log.h"
#include "core/io.h"
#include "core/input.h"
#include "scene/scene.h"
#include "render/resource.h"
#include "render/graphics_context.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_mesh_viewer.h"
#include "3rdparty/imgui_layer.h"

namespace z1 {

	RuntimeContext g_runtime_context;

	void RuntimeContext::init() {
		m_timer = std::make_shared<Timer>();
		m_logger = std::make_shared<Logger>();
		m_file_system = std::make_shared<FileSystem>();

		m_window = std::make_shared<Window>();
		m_window->init(Window::Config());

		m_input_system = std::make_shared<InputSystem>();

		m_graphics_context = GraphicsContext::create();
		m_graphics_context->init();

		m_resource_manager = std::make_shared<ResourceManager>();
		m_scene = std::make_shared<Scene>();

		m_imgui_layer = std::make_shared<ImGuiLayer>();
		m_layer_stack = std::make_shared<LayerStack>();

		m_renderer_2d = std::make_shared<Renderer2D>();
		m_renderer_mesh_viewer = std::make_shared<RendererMeshViewer>();
	}

	void RuntimeContext::shutdown() {

		m_main_camera.reset();
		m_main_framebuffer.reset();

		m_renderer_2d.reset();
		m_renderer_mesh_viewer.reset();

		m_layer_stack.reset();
		m_imgui_layer.reset();
		m_resource_manager.reset();
		m_graphics_context.reset();

		m_input_system.reset();
		m_window.reset();

		m_file_system.reset();
		m_logger.reset();
		m_timer.reset();
	}

}
