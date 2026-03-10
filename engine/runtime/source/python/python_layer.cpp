#include "pch.h"

#include "python/python_layer.h"
#include "core/core.h"
#include "core/window.h"
#include "core/application.h"

#include "event/key_event.h"
#include "event/mouse_event.h"
#include "event/application_event.h"

#include "pybind11/embed.h"
namespace py = pybind11;

#include <conio.h> // Windows specific for _kbhit and _getch

extern void ForceLinkPythonEngine();

namespace z1 {

	static std::multimap<EventType, py::object> s_python_event_listeners;

	void register_python_event_listener(EventType type, py::object callback) {
		s_python_event_listeners.insert({ type, callback });
	}

	PythonLayer::PythonLayer() : Layer("Python layer") {
		// temporary test space for python script runner
		ForceLinkPythonEngine();

		// 1. Initialize PyConfig
		PyConfig config;
		PyConfig_InitIsolatedConfig(&config); // Isolated means ignore environment variables

		// 2. Set the Python Home (the directory containing python314.zip or Lib/)
		// This replaces Py_SetPythonHome
		std::wstring home = std::filesystem::absolute("./pyenv").wstring();
		PyStatus status = PyConfig_SetString(&config, &config.home, home.c_str());
		if (PyStatus_Exception(status)) {
			PyConfig_Clear(&config);
			CORE_ASSERT(false, "Failed to set Python Home");
		}

		// Configure pycache prefix to redirect .pyc files to a central location
		std::wstring pycache_prefix = std::filesystem::absolute("./__pycache__").wstring();
		status = PyConfig_SetString(&config, &config.pycache_prefix, pycache_prefix.c_str());
		if (PyStatus_Exception(status)) {
			PyConfig_Clear(&config);
			CORE_ASSERT(false, "Failed to set pycache prefix");
		}

		// 3. Apply the configuration and initialize the interpreter
		status = Py_InitializeFromConfig(&config);
		if (PyStatus_Exception(status)) {
			CORE_ERROR("Python Init Error: {0}", status.err_msg);
			PyConfig_Clear(&config);
			CORE_ASSERT(false, "Failed to initialize Python interpreter");
		}
		PyConfig_Clear(&config); // Done with config, clear memory

		// 4. Now that Python is started, pybind11 can wrap it
		// We don't use scoped_interpreter here because we initialized manually
		try {
			py::exec(R"(
				import sys
				import os
				# print(f"Python Home: {sys.prefix}")
				sys.path.append(os.path.abspath("./content"))
				# print(f"Searching in: {sys.path}")

				import z1
				z1.log_info("python module import verified!")
			)");
		}
		catch (py::error_already_set& e) {
			CORE_ERROR("Python Error: {0}", e.what());
		}
	}

	PythonLayer::~PythonLayer() {
		CORE_DEBUG("shutting down PythonLayer ...");
		s_python_event_listeners.clear();
		// Cleanup manually at the end of the program
		Py_Finalize();
	}

	void PythonLayer::on_attach() {
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
		// Shutdown console thread
		m_running = false;
		if (m_console_thread->joinable()) {
			m_console_thread->join();
		}
	}

	void PythonLayer::on_event(Event& event) {
		auto range = s_python_event_listeners.equal_range(event.get_event_type());
		for (auto it = range.first; it != range.second; ++it) {
			try {
				// We need to cast event to specific type for python binding to work correctly?
				// Actually, pybind11 handles polymorphism if bound correctly.
				// Since Event is polymorphic base, passing &event should work if bindings are set up.
				it->second(&event);
			}
			catch (py::error_already_set& e) {
				CORE_ERROR("Python Event Listener Error: {0}", e.what());
			}
		}
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
