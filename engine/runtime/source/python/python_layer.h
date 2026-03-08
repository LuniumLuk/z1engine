#pragma once

#include "core/layer.h"

#include <queue>
#include <mutex>

namespace z1 {

	struct API PythonLayer : Layer {
		PythonLayer();
		~PythonLayer();

		void on_attach() override;
		void on_detach() override;

		void on_update(float) override;

	private:
		bool m_running = true;
		std::unique_ptr<std::thread> m_console_thread;
		std::queue<std::string> m_console_queue;
		std::mutex m_console_mutex;
	};

}
