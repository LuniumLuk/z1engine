#include "pch.h"
#include "z1engine.h"

#include "event/event.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "event/application_event.h"

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

	// Generated Bindings
	bind_generated(m, entity_cls);
}
