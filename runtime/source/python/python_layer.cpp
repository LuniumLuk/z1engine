#include "pch.h"

#include "python/python_layer.h"
#include "core/core.h"
#include "core/window.h"
#include "core/application.h"

#include "pybind11/embed.h"
namespace py = pybind11;

#include <conio.h> // Windows specific for _kbhit and _getch

extern void ForceLinkPythonEngine();

namespace z1 {

	PythonLayer::PythonLayer() : Layer("Python layer") {

	}

	PythonLayer::~PythonLayer() {

	}

	void PythonLayer::on_attach() {
		// temporary test space for python script runner
		ForceLinkPythonEngine();

		// 1. Initialize PyConfig
		PyConfig config;
		PyConfig_InitIsolatedConfig(&config); // Isolated means ignore environment variables

		// 2. Set the Python Home (the directory containing python314.zip or Lib/)
		// This replaces Py_SetPythonHome
		PyStatus status = PyConfig_SetString(&config, &config.home, L"./pyenv");
		if (PyStatus_Exception(status)) {
			PyConfig_Clear(&config);
			CORE_ASSERT(false, "Failed to set Python Home");
		}

		// 3. Apply the configuration and initialize the interpreter
		status = Py_InitializeFromConfig(&config);
		PyConfig_Clear(&config); // Done with config, clear memory
		if (PyStatus_Exception(status)) {
			CORE_ASSERT(false, "Failed to initialize Python interpreter");
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
			Engine.log_info("Python Path verified!")
		)");
		}
		catch (py::error_already_set& e) {
			CORE_ERROR("Python Error: {0}", e.what());
		}

		m_console_thread = std::make_unique<std::thread>([this]() {
			std::string line;
			while (m_running) {
				// 1. Check if a key has been pressed
				if (_kbhit()) {
					char c = _getch();

					if (c == '\r' || c == '\n') { // Enter key
						std::cout << std::endl; // Echo newline
						std::lock_guard<std::mutex> lock(m_console_mutex);
						m_console_queue.push(line);
						line.clear();
					}
					else if (c == '\b') { // Backspace
						if (!line.empty()) {
							line.pop_back();
							std::cout << "\b \b" << std::flush; // Erase char from console
						}
					}
					else {
						line += c;
						std::cout << c << std::flush; // Echo the character
					}
				} else {
					// 2. No key pressed, sleep briefly to save CPU, then check m_running
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			}
		});
	}

	void PythonLayer::on_detach() {
		// 5. Cleanup manually at the end of the program
		Py_Finalize();

		// 6. Shutdown console thread
		m_running = false;
		if (m_console_thread->joinable()) {
			m_console_thread->join();
		}
		CORE_DEBUG("shutting down PythonLayer ...");
	}

	void PythonLayer::on_update(float) {
		std::lock_guard<std::mutex> lock(m_console_mutex);
		while (!m_console_queue.empty()) {
			std::string line = m_console_queue.front();
			m_console_queue.pop();
			try {
				try {
					py::object result = py::eval(line);

					if (!result.is_none()) {
						std::string out = py::str(result);
						CORE_INFO(out);
					}
				}
				catch (py::error_already_set&) {
					py::exec(line);
				}
			}
			catch (py::error_already_set& e) {
				CORE_ERROR("Python Error: {0}", e.what());
			}
		}
	}

}
