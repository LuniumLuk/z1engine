#pragma once

#ifdef PLATFORM_WINDOWS
#    ifdef DYNAMIC_LINK
#        ifdef BUILD_DLL
#            define API __declspec(dllexport)
#        else
#            define API __declspec(dllimport)
#        endif
#    else
#        define API
#    endif
#else
#    error z1engine only support windows platform!
#endif

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

constexpr auto WHOLE_SIZE = ~0ULL;
constexpr auto NUM_MAX = ~0U;
constexpr auto INVALID_INDEX = ~0U;
constexpr auto INVALID_LOCATION = ~0U;
constexpr auto INVALID_BINDING = ~0U;

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

namespace z1 {

	struct Window;
	struct Timer;
	struct Logger;
	struct ImGuiLayer;
	struct LayerStack;
	struct Instrumentor;
	struct InstrumentationTimer;
	struct FileSystem;
	struct GraphicsContext;
	struct InputSystem;
	struct ResourceManager;
	struct Renderer2D;
	struct Renderer2DBatched;
	struct Camera;
	struct Framebuffer;
	struct Scene;

	struct API RuntimeContext {

		void init();
		void shutdown();

		std::shared_ptr<Window> m_window;
		std::shared_ptr<Timer> m_timer;
		std::shared_ptr<Logger> m_logger;
		std::shared_ptr<ImGuiLayer> m_imgui_layer;
		std::shared_ptr<LayerStack> m_layer_stack;
		std::shared_ptr<Instrumentor> m_instrumentor;
		std::shared_ptr<FileSystem> m_file_system;
		std::shared_ptr<GraphicsContext> m_graphics_context;
		std::shared_ptr<InputSystem> m_input_system;
		std::shared_ptr<ResourceManager> m_resource_manager;
		std::shared_ptr<Renderer2D> m_renderer_2d;
		std::shared_ptr<Scene> m_scene;

		std::shared_ptr<Camera> m_main_camera;
		std::shared_ptr<Framebuffer> m_main_framebuffer;

	};

	extern RuntimeContext g_runtime_context;

}

#include "core/log.h"

#define CORE_TRACE(...)    g_runtime_context.m_logger->get_core_logger()->trace(__VA_ARGS__)
#define CORE_DEBUG(...)    g_runtime_context.m_logger->get_core_logger()->debug(__VA_ARGS__)
#define CORE_INFO(...)     g_runtime_context.m_logger->get_core_logger()->info(__VA_ARGS__)
#define CORE_WARN(...)     g_runtime_context.m_logger->get_core_logger()->warn(__VA_ARGS__)
#define CORE_ERROR(...)    g_runtime_context.m_logger->get_core_logger()->error(__VA_ARGS__)
#define CORE_FATAL(...)    g_runtime_context.m_logger->get_core_logger()->critical(__VA_ARGS__)

#define CLIENT_TRACE(...)  g_runtime_context.m_logger->get_client_logger()->trace(__VA_ARGS__)
#define CLIENT_DEBUG(...)  g_runtime_context.m_logger->get_client_logger()->debug(__VA_ARGS__)
#define CLIENT_INFO(...)   g_runtime_context.m_logger->get_client_logger()->info(__VA_ARGS__)
#define CLIENT_WARN(...)   g_runtime_context.m_logger->get_client_logger()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...)  g_runtime_context.m_logger->get_client_logger()->error(__VA_ARGS__)
#define CLIENT_FATAL(...)  g_runtime_context.m_logger->get_client_logger()->critical(__VA_ARGS__)

#ifdef ENABLE_ASSERTS
#    define CORE_ASSERT(x, ...) { if(!(x)) { CORE_ERROR("assertion failed: {0}!", __VA_ARGS__); __debugbreak(); } }
#    define CLIENT_ASSERT(x, ...) { if(!(x)) { CLIENT_ERROR("assertion failed: {0}!", __VA_ARGS__); __debugbreak(); } }
#else
#    define CORE_ASSERT(x, ...)
#    define ASSERT(x, ...)
#endif

#define UNIMPLEMENTED_FUNCTION() CORE_WARN("{0} not implemented yet!", __FUNCSIG__)

#include "utils/instrumentor.h"

#ifdef ENABLE_PROFILE
#   define CONCAT(x, y) x ## y
#   define C(x, y) CONCAT(x, y)
#   define PROFILE_BEGIN_SESSION(name, filepath)  g_runtime_context.m_instrumentor->begin_session(name, filepath)
#   define PROFILE_END_SESSION()                  g_runtime_context.m_instrumentor->end_session()
#   define PROFILE_SET_THREAD_NAME(name)          g_runtime_context.m_instrumentor->set_thread_name(std::hash<std::thread::id>{}(std::this_thread::get_id()), name)
#   define PROFILE_SCOPE(name)                    z1::InstrumentationTimer C(__PROFILE_TIMER_, __LINE__)(name)
#   define PROFILE_COUNTER(name, value)           report_counter(name, static_cast<int64_t>(value))
#   define PROFILE_INSTANT(name)                  report_instant(name)
// Flow event can be used to represent the asynchronous dependencies between scope events
// The valid flow events should satisfy the following conditions:
//  1. The flow begin event should be called before a scope event's end on the same thread
//  2. The flow end event should be called before another scope event's begin on the same thread
// For example:
// // on thread 1:
// {
//     PROFILE_SCOPE("scope1");
//     // do something ...
//     PROFILE_FLOW_BEGIN(1); // !!must be called before the source scope event ends!!
// }
// // on thread 2:
// PROFILE_FLOW_END(1); // !!must be called before the target scope event starts!!
// {
//     PROFILE_SCOPE("scope2");
//     // do something ...
// }
#   define PROFILE_FLOW_BEGIN(id)                 report_flow(id, "s")
#   define PROFILE_FLOW_END(id)                   report_flow(id, "f")
#   define PROFILE_FUNCTION()                     PROFILE_SCOPE(__FUNCSIG__)
#else
#   define PROFILE_BEGIN_SESSION(name, filepath)
#   define PROFILE_END_SESSION()
#   define PROFILE_SET_THREAD_NAME(name)
#   define PROFILE_SCOPE(name)
#   define PROFILE_COUNTER(name, value)
#   define PROFILE_INSTANT(name)
#   define PROFILE_FLOW_BEGIN(id)
#   define PROFILE_FLOW_END(id)
#   define PROFILE_FUNCTION()
#endif
