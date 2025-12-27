#include "pch.h"

#include "z1engine.h"

#include "pybind11/embed.h"
namespace py = pybind11;

using namespace z1;

void ForceLinkPythonEngine() {}

void log_py(std::string const& msg) {
	CLIENT_INFO(msg);
}

// This macro "creates" the 'Engine' module inside the Python VM
PYBIND11_EMBEDDED_MODULE(Engine, m) {
	CORE_INFO("Initializing Engine Python module");
	m.doc() = "Internal Engine API";
	m.def("log", &log_py, "A function to log messages from Python to C++");
}
