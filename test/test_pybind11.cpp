#include "pybind11/embed.h"
#include <iostream>
#include <filesystem>

namespace py = pybind11;

// Simple function to expose
void log_to_console(std::string msg) {
	std::cout << "[Python]: " << msg << std::endl;
}

// Define the module
PYBIND11_EMBEDDED_MODULE(Engine, m) {
	m.def("log", &log_to_console);
}

int main() {

	auto cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	// 1. Initialize PyConfig
	PyConfig config;
	PyConfig_InitIsolatedConfig(&config); // Isolated means ignore environment variables

	// 2. Set the Python Home (the directory containing python314.zip or Lib/)
	// This replaces Py_SetPythonHome
	PyStatus status = PyConfig_SetString(&config, &config.home, L"../../pyenv");
	if (PyStatus_Exception(status)) {
		PyConfig_Clear(&config);
		return -1;
	}

	// 3. Apply the configuration and initialize the interpreter
	status = Py_InitializeFromConfig(&config);
	PyConfig_Clear(&config); // Done with config, clear memory
	if (PyStatus_Exception(status)) {
		return -1;
	}

	// 4. Now that Python is started, pybind11 can wrap it
	// We don't use scoped_interpreter here because we initialized manually
	try {
		py::exec(R"(
			import sys
			import os
			print(f"Python Home: {sys.prefix}")
			print(f"Searching in: {sys.path}")

			import Engine
			Engine.log("Python Path verified!")
		)");
	} catch (py::error_already_set& e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	// 5. Cleanup manually at the end of the program
	Py_Finalize();
	return 0;
}
