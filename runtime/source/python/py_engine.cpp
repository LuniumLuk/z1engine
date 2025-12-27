#include "pch.h"

#include "z1engine.h"

#include "pybind11/embed.h"
namespace py = pybind11;

using namespace z1;

void ForceLinkPythonEngine() {}

static void log_info_py(std::string const& msg) { CLIENT_INFO(msg); }
static void log_warn_py(std::string const& msg) { CLIENT_WARN(msg); }
static void log_error_py(std::string const& msg) { CLIENT_ERROR(msg); }

// This macro "creates" the 'Engine' module inside the Python VM
PYBIND11_EMBEDDED_MODULE(Engine, m) {
	CORE_INFO("Initializing Engine Python module");
	m.doc() = "Internal Engine API";
	m.def("log_info", &log_info_py);
	m.def("log_warn", &log_warn_py);
	m.def("log_error", &log_error_py);
}
