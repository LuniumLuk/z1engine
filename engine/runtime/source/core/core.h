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
	struct PythonLayer;
	struct LayerStack;
	struct InstrumentationTimer;
	struct FileSystem;
	struct GraphicsContext;
	struct InputSystem;
	struct ResourceManager;
	struct AssetManager;
	struct Renderer2D;
	struct RendererForward;
	struct GlobalSettings;

	struct API RuntimeContext {

		void init();
		void init_logger();
		void shutdown();

		std::shared_ptr<Window> m_window;
		std::shared_ptr<Timer> m_timer;
		std::shared_ptr<Logger> m_logger;
		std::shared_ptr<ImGuiLayer> m_imgui_layer;
		std::shared_ptr<PythonLayer> m_python_layer;
		std::shared_ptr<LayerStack> m_layer_stack;
		std::shared_ptr<FileSystem> m_file_system;
		std::shared_ptr<GraphicsContext> m_graphics_context;
		std::shared_ptr<InputSystem> m_input_system;
		std::shared_ptr<AssetManager> m_asset_manager;
		std::shared_ptr<Renderer2D> m_renderer_2d;
		std::shared_ptr<RendererForward> m_renderer_forward;
		std::shared_ptr<GlobalSettings> m_global;

	};

	extern RuntimeContext g_runtime_context;

}

#include "core/log.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define LOG_FILE_LINE
#ifdef LOG_FILE_LINE
#    define LOG_PREFIX "[" __FILE__ ":" TOSTRING(__LINE__) "] "
#else
#    ifdef LOG_FILE_LINE_FUNC
#        define LOG_PREFIX "[" __FILE__ ":" TOSTRING(__LINE__) " " __FUNCTION__ "] "
#    else
#        define LOG_PREFIX ""
#    endif
#endif

#define LOG_ARGS(fmt, ...)                                  fmt, ##__VA_ARGS__
#define LOG_ARGS_PREFIX(fmt, ...) std::string(LOG_PREFIX) + fmt, ##__VA_ARGS__

#define CORE_TRACE(fmt, ...)    g_runtime_context.m_logger->get_core_logger()->trace(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CORE_DEBUG(fmt, ...)    g_runtime_context.m_logger->get_core_logger()->debug(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CORE_INFO(fmt, ...)     g_runtime_context.m_logger->get_core_logger()->info(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CORE_WARN(fmt, ...)     g_runtime_context.m_logger->get_core_logger()->warn(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))
#define CORE_ERROR(fmt, ...)    g_runtime_context.m_logger->get_core_logger()->error(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))
#define CORE_FATAL(fmt, ...)    g_runtime_context.m_logger->get_core_logger()->critical(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))

#define CLIENT_TRACE(fmt, ...)  g_runtime_context.m_logger->get_client_logger()->trace(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CLIENT_DEBUG(fmt, ...)  g_runtime_context.m_logger->get_client_logger()->debug(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CLIENT_INFO(fmt, ...)   g_runtime_context.m_logger->get_client_logger()->info(LOG_ARGS(fmt, ##__VA_ARGS__))
#define CLIENT_WARN(fmt, ...)   g_runtime_context.m_logger->get_client_logger()->warn(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))
#define CLIENT_ERROR(fmt, ...)  g_runtime_context.m_logger->get_client_logger()->error(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))
#define CLIENT_FATAL(fmt, ...)  g_runtime_context.m_logger->get_client_logger()->critical(LOG_ARGS_PREFIX(fmt, ##__VA_ARGS__))

#ifdef ENABLE_ASSERTS
#    define CORE_ASSERT(x, ...) { if(!(x)) { CORE_ERROR("assertion failed: {0}!", __VA_ARGS__); __debugbreak(); } }
#    define ASSERT(x, ...) { if(!(x)) { CLIENT_ERROR("assertion failed: {0}!", __VA_ARGS__); __debugbreak(); } }
#else
#    define CORE_ASSERT(x, ...)
#    define ASSERT(x, ...)
#endif

#define CORE_ENSURE(x, ...) { if(!(x)) { CORE_ERROR("ensure failed: {0}!", __VA_ARGS__); } }
#define ENSURE(x, ...) { if(!(x)) { CLIENT_ERROR("ensure failed: {0}!", __VA_ARGS__); } }

#define UNIMPLEMENTED_FUNCTION() CORE_WARN("{0} not implemented yet!", __FUNCSIG__)

#include "util/instrumentor.h"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT3(a, b, c) CONCAT(CONCAT(a, b), c)

#ifdef ENABLE_PROFILE
#    define PROFILE_BEGIN_SESSION(name, filepath)  z1::Instrumentor::get().begin_session(name, filepath)
#    define PROFILE_END_SESSION()                  z1::Instrumentor::get().end_session()
#    define PROFILE_SET_THREAD_NAME(name)          z1::Instrumentor::get().set_thread_name(std::hash<std::thread::id>{}(std::this_thread::get_id()), name)
#    define PROFILE_SCOPE(name)                    z1::InstrumentationTimer CONCAT(__PROFILE_TIMER_, __LINE__)(name)
#    define PROFILE_COUNTER(name, value)           z1::report_counter(name, static_cast<int64_t>(value))
#    define PROFILE_INSTANT(name)                  z1::report_instant(name)
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
#    define PROFILE_FLOW_BEGIN(id)                 z1::report_flow(id, "s")
#    define PROFILE_FLOW_END(id)                   z1::report_flow(id, "f")
#    define PROFILE_FUNCTION()                     PROFILE_SCOPE(__FUNCSIG__)
#else
#    define PROFILE_BEGIN_SESSION(name, filepath)
#    define PROFILE_END_SESSION()
#    define PROFILE_SET_THREAD_NAME(name)
#    define PROFILE_SCOPE(name)
#    define PROFILE_COUNTER(name, value)
#    define PROFILE_INSTANT(name)
#    define PROFILE_FLOW_BEGIN(id)
#    define PROFILE_FLOW_END(id)
#    define PROFILE_FUNCTION()
#endif

#include "core/reflection.h"

#define REFLECTED_STRUCT(type)                                         \
	struct _REFLECT_REGISTER_##type {                                  \
		_REFLECT_REGISTER_##type() {                                   \
			TypeRegistry::instance().register_type(#type);             \
		}                                                              \
	};                                                                 \
	static _REFLECT_REGISTER_##type _REFLECT_REGISTER_INSTANCE_##type; \
	struct API type

#define REFLECTED_FIELD(type, field, ...)                              \
	struct CONCAT3(_REFLECT_REGISTER_, type, _##field) {               \
		CONCAT3(_REFLECT_REGISTER_, type, _##field)() {                \
			FieldInfo field_info = {                                   \
				#field,                                                \
				offsetof(type, field),                                 \
				sizeof(((type*)0)->field),                             \
				&typeid(((type*)0)->field),                            \
				__VA_ARGS__                                            \
			};                                                         \
			field_info.container = z1::ContainerInfoResolver<std::remove_cv_t<std::remove_reference_t<decltype(((type*)0)->field)>>>::get(); \
			TypeRegistry::instance().register_field(#type, field_info);\
		}                                                              \
	};                                                                 \
	static CONCAT3(_REFLECT_REGISTER_, type, _##field) CONCAT3(_REFLECT_REGISTER_INSTANCE_, type, _##field);

#define DISABLE_COPY(type) \
	type(type const&) = delete; \
	type& operator=(type const&) = delete;

#define TYPE_NAME(type) #type

#ifdef DEBUG
#define DEBUG_CHECK(expr, ...)                                         \
	{                                                                  \
		if (!(expr)) {                                                 \
			CORE_ERROR("debug check failed: {0}", __VA_ARGS__);        \
			__debugbreak();                                            \
		}                                                              \
	}
#else
#define DEBUG_CHECK(expr, ...)
#endif
