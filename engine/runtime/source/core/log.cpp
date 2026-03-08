#include "pch.h"
#include "core/log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace z1 {

	Logger::Logger() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		m_core_logger = spdlog::stdout_color_mt("CORE");
		m_core_logger->set_level(spdlog::level::trace);
		m_client_logger = spdlog::stdout_color_mt("CLIENT");
		m_client_logger->set_level(spdlog::level::trace);
	}

}
