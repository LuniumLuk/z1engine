#pragma once

#include "core/core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace z1 {

	enum struct API LogLevel : int {
		Trace = 0,
		Debug,
		Info,
		Warn,
		Error,
		Critical,
		Off
	};

	static_assert((int)LogLevel::Trace == (int)spdlog::level::trace);
	static_assert((int)LogLevel::Debug == (int)spdlog::level::debug);
	static_assert((int)LogLevel::Info == (int)spdlog::level::info);
	static_assert((int)LogLevel::Warn == (int)spdlog::level::warn);
	static_assert((int)LogLevel::Error == (int)spdlog::level::err);
	static_assert((int)LogLevel::Critical == (int)spdlog::level::critical);
	static_assert((int)LogLevel::Off == (int)spdlog::level::off);

	struct API Logger {
		Logger();

		inline std::shared_ptr<spdlog::logger>& get_core_logger() { return m_core_logger; }
		inline std::shared_ptr<spdlog::logger>& get_client_logger() { return m_client_logger; }

		void set_core_log_level(LogLevel level);
		void set_client_log_level(LogLevel level);

	private:
		std::shared_ptr<spdlog::logger> m_core_logger;
		std::shared_ptr<spdlog::logger> m_client_logger;
	};

}
