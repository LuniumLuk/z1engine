#include "pch.h"

#include "z1engine.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "core/input.h"

#include "pybind11/embed.h"
namespace py = pybind11;

using namespace z1;

void ForceLinkPythonEngine() {}

static void log_info_py(std::string const& msg) { CLIENT_INFO(msg); }
static void log_warn_py(std::string const& msg) { CLIENT_WARN(msg); }
static void log_error_py(std::string const& msg) { CLIENT_ERROR(msg); }

// This macro "creates" the 'z1' module inside the Python VM
PYBIND11_EMBEDDED_MODULE(z1, m) {
	CORE_INFO("Initializing z1 Python module");
	m.doc() = "z1 Engine API";
	m.def("log_info", &log_info_py);
	m.def("log_warn", &log_warn_py);
	m.def("log_error", &log_error_py);

	// Bind glm::vec3
	py::class_<glm::vec3>(m, "Vec3")
		.def(py::init<float, float, float>())
		.def(py::init<>())
		.def_readwrite("x", &glm::vec3::x)
		.def_readwrite("y", &glm::vec3::y)
		.def_readwrite("z", &glm::vec3::z)
		.def("__repr__", [](const glm::vec3& v) {
			return "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
		});

	// Bind TransformComponent
	py::class_<TransformComponent>(m, "Transform")
		.def_readwrite("location", &TransformComponent::m_location)
		.def_readwrite("rotation", &TransformComponent::m_rotation)
		.def_readwrite("scale", &TransformComponent::m_scale);

	// Bind Entity
	py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity")
		.def_property_readonly("transform", [](Entity& e) -> TransformComponent& {
			return e.get_component<TransformComponent>();
		}, py::return_value_policy::reference)
		.def("is_valid", &Entity::is_valid);

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
}
