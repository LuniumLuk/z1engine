#include "pch.h"
#include "z1engine.h"

#include "event/event.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "event/application_event.h"
#include "core/input.h"

#include "pybind11/embed.h"
namespace py = pybind11;

#include <vector>
#include <map>

namespace z1 {
	void register_python_event_listener(EventType type, py::object callback);
}

using namespace z1;

void ForceLinkPythonEngine() {}

static void log_info_py(std::string const& msg) { CLIENT_INFO(msg); }
static void log_warn_py(std::string const& msg) { CLIENT_WARN(msg); }
static void log_error_py(std::string const& msg) { CLIENT_ERROR(msg); }

// Declaration of generated function
void bind_generated(py::module& m, py::class_<Entity, std::shared_ptr<Entity>>& entity_cls);

// This macro "creates" the 'z1' module inside the Python VM
PYBIND11_EMBEDDED_MODULE(z1, m) {
	CORE_INFO("initializing z1 python module");
	m.doc() = "z1 Engine API";
	m.def("log_info", &log_info_py);
	m.def("log_warn", &log_warn_py);
	m.def("log_error", &log_error_py);

	// Cursor control
	m.def("hide_cursor", []() { g_runtime_context.m_window->hide_cursor(); });
	m.def("show_cursor", []() { g_runtime_context.m_window->show_cursor(); });
	m.def("center_cursor", []() { g_runtime_context.m_window->center_cursor(); });
	m.def("is_cursor_hidden", []() -> bool { return g_runtime_context.m_window->is_cursor_hidden(); });

	// Bind Event System
	py::enum_<EventType>(m, "EventType")
		.value("WindowClose", EventType::WindowClose)
		.value("WindowResize", EventType::WindowResize)
		.value("WindowFocus", EventType::WindowFocus)
		.value("WindowLostFocus", EventType::WindowLostFocus)
		.value("WindowMoved", EventType::WindowMoved)
		.value("KeyPressed", EventType::KeyPressed)
		.value("KeyReleased", EventType::KeyReleased)
		.value("KeyTyped", EventType::KeyTyped)
		.value("MouseButtonPressed", EventType::MouseButtonPressed)
		.value("MouseButtonReleased", EventType::MouseButtonReleased)
		.value("MouseMoved", EventType::MouseMoved)
		.value("MouseScrolled", EventType::MouseScrolled)
		.export_values();

	py::class_<Event>(m, "Event")
		.def("get_event_type", &Event::get_event_type)
		.def("get_name", &Event::get_name)
		.def("__repr__", &Event::to_string);

	py::class_<KeyEvent, Event>(m, "KeyEvent")
		.def_property_readonly("key_code", &KeyEvent::get_keycode);

	py::class_<KeyPressedEvent, KeyEvent>(m, "KeyPressedEvent")
		.def_property_readonly("repeat_count", &KeyPressedEvent::get_repeat_count);

	py::class_<KeyReleasedEvent, KeyEvent>(m, "KeyReleasedEvent");

	py::class_<KeyTypedEvent, KeyEvent>(m, "KeyTypedEvent")
		.def_property_readonly("key_code", &KeyTypedEvent::get_keycode);

	py::class_<MouseButtonEvent, Event>(m, "MouseButtonEvent")
		.def_property_readonly("mouse_button", &MouseButtonEvent::get_button);

	py::class_<MouseButtonPressedEvent, MouseButtonEvent>(m, "MouseButtonPressedEvent");
	py::class_<MouseButtonReleasedEvent, MouseButtonEvent>(m, "MouseButtonReleasedEvent");

	py::class_<MouseMovedEvent, Event>(m, "MouseMovedEvent")
		.def_property_readonly("x", &MouseMovedEvent::get_x)
		.def_property_readonly("y", &MouseMovedEvent::get_y);

	py::class_<MouseScrollEvent, Event>(m, "MouseScrollEvent")
		.def_property_readonly("x_offset", &MouseScrollEvent::get_x_offset)
		.def_property_readonly("y_offset", &MouseScrollEvent::get_y_offset);

	py::class_<WindowResizeEvent, Event>(m, "WindowResizeEvent")
		.def_property_readonly("width", &WindowResizeEvent::get_width)
		.def_property_readonly("height", &WindowResizeEvent::get_height);

	py::class_<WindowCloseEvent, Event>(m, "WindowCloseEvent");


	m.def("register_event_listener", &register_python_event_listener);


	// Bind GLM
	py::class_<glm::vec2>(m, "Vec2")
		.def(py::init<float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec2::x)
		.def_readwrite("y", &glm::vec2::y)
		.def("__repr__", [](glm::vec2 const& v) {
			return "Vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
		});

	py::class_<glm::vec3>(m, "Vec3")
		.def(py::init<float, float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec3::x)
		.def_readwrite("y", &glm::vec3::y)
		.def_readwrite("z", &glm::vec3::z)
		.def("__repr__", [](glm::vec3 const& v) {
			return "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
		});

	py::class_<glm::vec4>(m, "Vec4")
		.def(py::init<float, float, float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec4::x)
		.def_readwrite("y", &glm::vec4::y)
		.def_readwrite("z", &glm::vec4::z)
		.def_readwrite("w", &glm::vec4::w)
		.def("__repr__", [](glm::vec4 const& v) {
			return "Vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
		});

	// Bind Entity
	// Note: Component properties are added in bind_generated
	auto entity_cls = py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity");
	entity_cls
		.def("is_valid", &Entity::is_valid)
		.def("add_static_mesh", [](Entity& self, std::string const& path) {
			if (!self.has_component<StaticMeshComponent>()) {
				auto mesh = g_runtime_context.m_asset_manager->get<StaticMesh>(path);
				self.add_component<StaticMeshComponent>(mesh);
			}
			else {
				CLIENT_WARN("Entity already has a StaticMeshComponent, skipping add_static_mesh");
			}
		})
		.def("add_skeletal_mesh", [](Entity& self, std::string const& path) {
			if (!self.has_component<SkeletalMeshComponent>()) {
				auto mesh = g_runtime_context.m_asset_manager->get<SkeletalMesh>(path);
				self.add_component<SkeletalMeshComponent>(mesh);
			}
			else {
				CLIENT_WARN("Entity already has a SkeletalMeshComponent, skipping add_static_mesh");
			}
		})
		.def("add_camera", [](Entity& self) {
			if (!self.has_component<CameraComponent>()) {
				self.add_component<CameraComponent>();
			}
			else {
				CLIENT_WARN("Entity already has a CameraComponent, skipping add_camera");
			}
		})
		.def("add_script", [](Entity& self, std::string const& module, std::string const& cls) {
			if (!self.has_component<ScriptComponent>()) {
				self.attach_script<PythonScript>(module, cls);
			}
		});

	// Helper class for Python to inherit from (mocking ScriptBase)
	struct PyScript {
		virtual void on_attach() {}
		virtual void on_start() {}
		virtual void on_update(float) {}
		virtual void on_destroy() {}
		virtual void on_detach() {}
		virtual ~PyScript() = default;
	};

	// Bind Script Base Class for Python to inherit
	py::class_<PyScript>(m, "Script")
		.def(py::init<>())
		.def("on_attach", &PyScript::on_attach)
		.def("on_start", &PyScript::on_start)
		.def("on_update", &PyScript::on_update)
		.def("on_destroy", &PyScript::on_destroy)
		.def("on_detach", &PyScript::on_detach);

	py::class_<Scene, std::shared_ptr<Scene>>(m, "Scene")
		.def(py::init<>())
		.def("create_entity", &Scene::create_entity)
		.def("destroy_entity", &Scene::destroy_entity);

	// Input polling submodule
	auto input_module = m.def_submodule("input", "Input polling API");
	input_module.def("is_key_pressed", [](int keycode) -> bool {
		if (!g_runtime_context.m_input_system) return false;
		return g_runtime_context.m_input_system->is_key_pressed(keycode);
	});
	input_module.def("is_mouse_button_pressed", [](int button) -> bool {
		if (!g_runtime_context.m_input_system) return false;
		return g_runtime_context.m_input_system->is_mouse_button_pressed(button);
	});
	input_module.def("get_mouse_pos", []() -> std::pair<float, float> {
		if (!g_runtime_context.m_input_system) return {0.0f, 0.0f};
		return g_runtime_context.m_input_system->get_mouse_pos();
	});

	// GLFW key code constants (exposed on z1.input)
	#define ADD_KEY_CONST(name, code) input_module.attr(#name) = code
	ADD_KEY_CONST(KEY_SPACE,        32);
	ADD_KEY_CONST(KEY_APOSTROPHE,   39);
	ADD_KEY_CONST(KEY_COMMA,        44);
	ADD_KEY_CONST(KEY_MINUS,        45);
	ADD_KEY_CONST(KEY_PERIOD,       46);
	ADD_KEY_CONST(KEY_SLASH,        47);
	ADD_KEY_CONST(KEY_0,            48);
	ADD_KEY_CONST(KEY_1,            49);
	ADD_KEY_CONST(KEY_2,            50);
	ADD_KEY_CONST(KEY_3,            51);
	ADD_KEY_CONST(KEY_4,            52);
	ADD_KEY_CONST(KEY_5,            53);
	ADD_KEY_CONST(KEY_6,            54);
	ADD_KEY_CONST(KEY_7,            55);
	ADD_KEY_CONST(KEY_8,            56);
	ADD_KEY_CONST(KEY_9,            57);
	ADD_KEY_CONST(KEY_SEMICOLON,    59);
	ADD_KEY_CONST(KEY_EQUAL,        61);
	ADD_KEY_CONST(KEY_A,            65);
	ADD_KEY_CONST(KEY_B,            66);
	ADD_KEY_CONST(KEY_C,            67);
	ADD_KEY_CONST(KEY_D,            68);
	ADD_KEY_CONST(KEY_E,            69);
	ADD_KEY_CONST(KEY_F,            70);
	ADD_KEY_CONST(KEY_G,            71);
	ADD_KEY_CONST(KEY_H,            72);
	ADD_KEY_CONST(KEY_I,            73);
	ADD_KEY_CONST(KEY_J,            74);
	ADD_KEY_CONST(KEY_K,            75);
	ADD_KEY_CONST(KEY_L,            76);
	ADD_KEY_CONST(KEY_M,            77);
	ADD_KEY_CONST(KEY_N,            78);
	ADD_KEY_CONST(KEY_O,            79);
	ADD_KEY_CONST(KEY_P,            80);
	ADD_KEY_CONST(KEY_Q,            81);
	ADD_KEY_CONST(KEY_R,            82);
	ADD_KEY_CONST(KEY_S,            83);
	ADD_KEY_CONST(KEY_T,            84);
	ADD_KEY_CONST(KEY_U,            85);
	ADD_KEY_CONST(KEY_V,            86);
	ADD_KEY_CONST(KEY_W,            87);
	ADD_KEY_CONST(KEY_X,            88);
	ADD_KEY_CONST(KEY_Y,            89);
	ADD_KEY_CONST(KEY_Z,            90);
	ADD_KEY_CONST(KEY_LEFT_BRACKET, 91);
	ADD_KEY_CONST(KEY_BACKSLASH,    92);
	ADD_KEY_CONST(KEY_RIGHT_BRACKET,93);
	ADD_KEY_CONST(KEY_GRAVE_ACCENT, 96);
	ADD_KEY_CONST(KEY_ESCAPE,       256);
	ADD_KEY_CONST(KEY_ENTER,        257);
	ADD_KEY_CONST(KEY_TAB,          258);
	ADD_KEY_CONST(KEY_BACKSPACE,    259);
	ADD_KEY_CONST(KEY_INSERT,       260);
	ADD_KEY_CONST(KEY_DELETE,       261);
	ADD_KEY_CONST(KEY_RIGHT,        262);
	ADD_KEY_CONST(KEY_LEFT,         263);
	ADD_KEY_CONST(KEY_DOWN,         264);
	ADD_KEY_CONST(KEY_UP,           265);
	ADD_KEY_CONST(KEY_PAGE_UP,      266);
	ADD_KEY_CONST(KEY_PAGE_DOWN,    267);
	ADD_KEY_CONST(KEY_HOME,         268);
	ADD_KEY_CONST(KEY_END,          269);
	ADD_KEY_CONST(KEY_CAPS_LOCK,    280);
	ADD_KEY_CONST(KEY_SCROLL_LOCK,  281);
	ADD_KEY_CONST(KEY_NUM_LOCK,     282);
	ADD_KEY_CONST(KEY_PRINT_SCREEN, 283);
	ADD_KEY_CONST(KEY_PAUSE,        284);
	ADD_KEY_CONST(KEY_F1,           290);
	ADD_KEY_CONST(KEY_F2,           291);
	ADD_KEY_CONST(KEY_F3,           292);
	ADD_KEY_CONST(KEY_F4,           293);
	ADD_KEY_CONST(KEY_F5,           294);
	ADD_KEY_CONST(KEY_F6,           295);
	ADD_KEY_CONST(KEY_F7,           296);
	ADD_KEY_CONST(KEY_F8,           297);
	ADD_KEY_CONST(KEY_F9,           298);
	ADD_KEY_CONST(KEY_F10,          299);
	ADD_KEY_CONST(KEY_F11,          300);
	ADD_KEY_CONST(KEY_F12,          301);
	ADD_KEY_CONST(KEY_KP_0,         320);
	ADD_KEY_CONST(KEY_KP_1,         321);
	ADD_KEY_CONST(KEY_KP_2,         322);
	ADD_KEY_CONST(KEY_KP_3,         323);
	ADD_KEY_CONST(KEY_KP_4,         324);
	ADD_KEY_CONST(KEY_KP_5,         325);
	ADD_KEY_CONST(KEY_KP_6,         326);
	ADD_KEY_CONST(KEY_KP_7,         327);
	ADD_KEY_CONST(KEY_KP_8,         328);
	ADD_KEY_CONST(KEY_KP_9,         329);
	ADD_KEY_CONST(KEY_KP_DECIMAL,   330);
	ADD_KEY_CONST(KEY_KP_DIVIDE,    331);
	ADD_KEY_CONST(KEY_KP_MULTIPLY,  332);
	ADD_KEY_CONST(KEY_KP_SUBTRACT,  333);
	ADD_KEY_CONST(KEY_KP_ADD,       334);
	ADD_KEY_CONST(KEY_KP_ENTER,     335);
	ADD_KEY_CONST(KEY_KP_EQUAL,     336);
	ADD_KEY_CONST(KEY_LEFT_SHIFT,   340);
	ADD_KEY_CONST(KEY_LEFT_CONTROL, 341);
	ADD_KEY_CONST(KEY_LEFT_ALT,     342);
	ADD_KEY_CONST(KEY_LEFT_SUPER,   343);
	ADD_KEY_CONST(KEY_RIGHT_SHIFT,  344);
	ADD_KEY_CONST(KEY_RIGHT_CONTROL,345);
	ADD_KEY_CONST(KEY_RIGHT_ALT,    346);
	ADD_KEY_CONST(KEY_RIGHT_SUPER,  347);
	#undef ADD_KEY_CONST

	// Mouse button constants
	#define ADD_MOUSE_CONST(name, code) input_module.attr(#name) = code
	ADD_MOUSE_CONST(MOUSE_BUTTON_1,      0);
	ADD_MOUSE_CONST(MOUSE_BUTTON_2,      1);
	ADD_MOUSE_CONST(MOUSE_BUTTON_3,      2);
	ADD_MOUSE_CONST(MOUSE_BUTTON_4,      3);
	ADD_MOUSE_CONST(MOUSE_BUTTON_5,      4);
	ADD_MOUSE_CONST(MOUSE_BUTTON_6,      5);
	ADD_MOUSE_CONST(MOUSE_BUTTON_7,      6);
	ADD_MOUSE_CONST(MOUSE_BUTTON_8,      7);
	ADD_MOUSE_CONST(MOUSE_BUTTON_LEFT,   0);
	ADD_MOUSE_CONST(MOUSE_BUTTON_RIGHT,  1);
	ADD_MOUSE_CONST(MOUSE_BUTTON_MIDDLE, 2);
	#undef ADD_MOUSE_CONST

	// Generated Bindings
	bind_generated(m, entity_cls);
}
