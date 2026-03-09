#include "pch.h"
#include "z1engine.h"
#include "render/global.h"

#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

using namespace z1;

void ForceLinkPythonEngine() {}

static void log_info_py(std::string const& msg) { CLIENT_INFO(msg); }
static void log_warn_py(std::string const& msg) { CLIENT_WARN(msg); }
static void log_error_py(std::string const& msg) { CLIENT_ERROR(msg); }

// Declaration of generated function
void bind_generated(py::module& m, py::class_<Entity, std::shared_ptr<Entity>>& entity_cls);

// This macro "creates" the 'z1' module inside the Python VM
PYBIND11_EMBEDDED_MODULE(z1, m) {
	CORE_INFO("Initializing z1 Python module");
	m.doc() = "z1 Engine API";
	m.def("log_info", &log_info_py);
	m.def("log_warn", &log_warn_py);
	m.def("log_error", &log_error_py);

	// Bind GLM
	py::class_<glm::vec2>(m, "Vec2")
		.def(py::init<float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec2::x)
		.def_readwrite("y", &glm::vec2::y)
		.def("__repr__", [](const glm::vec2& v) {
			return "Vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
		});

	py::class_<glm::vec3>(m, "Vec3")
		.def(py::init<float, float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec3::x)
		.def_readwrite("y", &glm::vec3::y)
		.def_readwrite("z", &glm::vec3::z)
		.def("__repr__", [](const glm::vec3& v) {
			return "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
		});

	py::class_<glm::vec4>(m, "Vec4")
		.def(py::init<float, float, float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec4::x)
		.def_readwrite("y", &glm::vec4::y)
		.def_readwrite("z", &glm::vec4::z)
		.def_readwrite("w", &glm::vec4::w)
		.def("__repr__", [](const glm::vec4& v) {
			return "Vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
		});

	// Bind Entity
	// Note: Component properties are added in bind_generated
	auto entity_cls = py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity");
	entity_cls.def("is_valid", &Entity::is_valid);

	// Helper class for Python to inherit from (mocking ScriptBase)
	struct PyScript {
		virtual void on_attach() {}
		virtual void on_update(float) {}
		virtual void on_detach() {}
		virtual ~PyScript() = default;
	};

	// Bind Script Base Class for Python to inherit
	py::class_<PyScript>(m, "Script")
		.def(py::init<>())
		.def("on_attach", &PyScript::on_attach)
		.def("on_update", &PyScript::on_update)
		.def("on_detach", &PyScript::on_detach);

	py::class_<Scene, std::shared_ptr<Scene>>(m, "Scene")
		.def(py::init<>())
		.def("create_entity", &Scene::create_entity)
		.def("destroy_entity", &Scene::destroy_entity);

	// Generated Bindings
	bind_generated(m, entity_cls);
}
