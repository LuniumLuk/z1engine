#include "pch.h"
#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/timer.h"
#include "core/log.h"
#include "utils/instrumentor.h"
#include "3rdparty/imgui_layer.h"
#include "core/io.h"
#include "render/graphics_context.h"
#include "core/input.h"
#include "render/resource.h"

namespace z1 {

    RuntimeContext g_runtime_context;

    void RuntimeContext::init() {
        m_timer = std::make_shared<Timer>();
        m_logger = std::make_shared<Logger>();
        m_instrumentor = std::make_shared<Instrumentor>();
        m_file_system = std::make_shared<FileSystem>();

        m_window = std::make_shared<Window>();
        m_window->init(Window::Config());

        m_input_system = std::make_shared<InputSystem>();

        m_graphics_context = GraphicsContext::create();
        m_graphics_context->init();

        m_resource_manager = std::make_shared<ResourceManager>();

        m_imgui_layer = std::make_shared<ImGuiLayer>();
        m_layer_stack = std::make_shared<LayerStack>();
    }

    void RuntimeContext::shutdown() {
        m_window.reset();
        m_timer.reset();
        m_logger.reset();
        m_imgui_layer.reset();
        m_layer_stack.reset();
        m_instrumentor.reset();
        m_file_system.reset();
        m_graphics_context.reset();
        m_input_system.reset();
        m_resource_manager.reset();
    }

}
