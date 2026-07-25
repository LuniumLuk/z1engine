#ifdef PLATFORM_WINDOWS
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
	auto python_home = std::filesystem::absolute(cwd / "engine" / "3rdparty" / "python314");
	if (!std::filesystem::exists(python_home / "python314.zip")) {
		std::cout << "Embedded Python home not found: " << python_home.generic_string() << std::endl;
		return 1;
	}

	// 1. Initialize PyConfig
	PyConfig config;
	PyConfig_InitIsolatedConfig(&config); // Isolated means ignore environment variables

	// 2. Set the Python Home (the directory containing python314.zip or Lib/)
	// This replaces Py_SetPythonHome
	PyStatus status = PyConfig_SetString(&config, &config.home, python_home.wstring().c_str());
	if (PyStatus_Exception(status)) {
		PyConfig_Clear(&config);
		return -1;
	}

	config.module_search_paths_set = 1;
	auto python_zip = (python_home / "python314.zip").wstring();
	status = PyWideStringList_Append(&config.module_search_paths, python_zip.c_str());
	if (PyStatus_Exception(status)) {
		PyConfig_Clear(&config);
		return -1;
	}
	status = PyWideStringList_Append(&config.module_search_paths, python_home.wstring().c_str());
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
	bool ok = false;
	try {
		py::exec(R"(
			import sys
			import os
			print(f"Python Home: {sys.prefix}")
			print(f"Searching in: {sys.path}")

			import Engine
			Engine.log("Python Path verified!")
		)");
		ok = true;
	} catch (py::error_already_set& e) {
		std::cout << "Error: " << e.what() << std::endl;
		Py_Finalize();
		return 1;
	}

	// 5. Cleanup manually at the end of the program
	Py_Finalize();
	return ok ? 0 : 1;
}

#endif // PLATFORM_WINDOWS
