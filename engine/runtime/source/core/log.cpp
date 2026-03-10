#include "pch.h"
#include "core/log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace z1 {

	Logger::Logger() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		m_core_logger = spdlog::stdout_color_mt("CORE");
		m_core_logger->set_level(spdlog::level::info);
		m_client_logger = spdlog::stdout_color_mt("CLIENT");
		m_client_logger->set_level(spdlog::level::info);
	}

	void Logger::set_core_log_level(LogLevel level) {
		m_core_logger->set_level(static_cast<spdlog::level::level_enum>(level));
	}

	void Logger::set_client_log_level(LogLevel level) {
		m_client_logger->set_level(static_cast<spdlog::level::level_enum>(level));
	}

}
