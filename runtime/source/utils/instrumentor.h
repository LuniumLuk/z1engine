#pragma once

#include "core/core.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <thread>
#include <unordered_map>

// to enable profiling, define ENABLE_PROFILE before including this file.
// - to start a session, use PROFILE_BEGIN_SESSION("name", "filepath")
// - to end a session, use PROFILE_END_SESSION()
// - to profile a function, use PROFILE_FUNCTION()
// - to profile a scope, use PROFILE_SCOPE("name")

// to see the results, open the generated json file in chrome://tracing

//#define ENABLE_PROFILE

namespace z1 {

	struct ProfileResult {
		std::string name;
		int64_t start, end;
		size_t thread_id;
	};

	struct CounterResult {
		std::string name;
		int64_t time;
		size_t thread_id;
		int64_t value;
	};

	struct InstantResult {
		std::string name;
		int64_t time;
		size_t thread_id;
	};

	struct FlowResult {
		int64_t time;
		size_t thread_id;
		size_t id;
		std::string phase;
	};

	struct InstrumentationSession {
		std::string name;
	};

	struct Instrumentor {
		static Instrumentor& get() {
			static Instrumentor instance;
			return instance;
		}

		Instrumentor() : m_current_session(nullptr), m_profile_count(0) {}

		void begin_session(std::string const& name, std::string const& filepath = "profile.json") {
			std::lock_guard<std::mutex> lock(m_mutex);

			m_ostream.open(filepath);
			m_filepath = filepath;

			write_header();
			m_current_session = new InstrumentationSession{ name };
			m_profile_count = 0;
		}

		void end_session() {
			std::lock_guard<std::mutex> lock(m_mutex);

			write_footer();
			m_ostream.close();
			delete m_current_session;
			m_current_session = nullptr;
			m_profile_count = 0;
		}

		void write_profile(ProfileResult const& result) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_profile_count++ > 0)
				m_ostream << ",";

			std::string name = result.name;
			std::replace(name.begin(), name.end(), '"', '\'');

			m_ostream << "{";
			m_ostream << "\"cat\":\"function\",";
			m_ostream << "\"dur\":" << (result.end - result.start) << ',';
			m_ostream << "\"name\":\"" << name << "\",";
			m_ostream << "\"ph\":\"X\",";
			m_ostream << "\"pid\":0,";
			if (m_thread_names.find(result.thread_id) != m_thread_names.end())
				m_ostream << "\"tid\":\"" << m_thread_names[result.thread_id] << "\",";
			else
				m_ostream << "\"tid\":" << result.thread_id << ",";
			m_ostream << "\"ts\":" << result.start;
			m_ostream << "}";

			m_ostream.flush();
		}

		void write_counter(CounterResult const& result) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_profile_count++ > 0)
				m_ostream << ",";

			std::string name = result.name;
			std::replace(name.begin(), name.end(), '"', '\'');

			m_ostream << "{";
			m_ostream << "\"cat\":\"counter\",";
			m_ostream << "\"name\":\"" << name << "\",";
			m_ostream << "\"ph\":\"C\",";
			m_ostream << "\"pid\":0,";
			if (m_thread_names.find(result.thread_id) != m_thread_names.end())
				m_ostream << "\"tid\":\"" << m_thread_names[result.thread_id] << "\",";
			else
				m_ostream << "\"tid\":" << result.thread_id << ",";
			m_ostream << "\"ts\":" << result.time << ",";
			m_ostream << "\"args\":{\"" << result.name << "\":" << result.value << "}";
			m_ostream << "}";

			m_ostream.flush();
		}

		void write_instant(InstantResult const& result) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_profile_count++ > 0)
				m_ostream << ",";

			std::string name = result.name;
			std::replace(name.begin(), name.end(), '"', '\'');

			m_ostream << "{";
			m_ostream << "\"cat\":\"event\",";
			m_ostream << "\"name\":\"" << name << "\",";
			m_ostream << "\"ph\":\"i\",";
			m_ostream << "\"pid\":0,";
			if (m_thread_names.find(result.thread_id) != m_thread_names.end())
				m_ostream << "\"tid\":\"" << m_thread_names[result.thread_id] << "\",";
			else
				m_ostream << "\"tid\":" << result.thread_id << ",";
			m_ostream << "\"ts\":" << result.time << ",";
			m_ostream << "\"s\":\"g\"";
			m_ostream << "}";

			m_ostream.flush();
		}

		void write_flow(FlowResult const& result) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_profile_count++ > 0)
				m_ostream << ",";

			m_ostream << "{";
			m_ostream << "\"cat\":\"function\",";
			m_ostream << "\"name\":\"flow\",";
			m_ostream << "\"ph\":\"" << result.phase << "\",";
			m_ostream << "\"id\":\"" << result.id << "\",";
			m_ostream << "\"pid\":0,";
			if (m_thread_names.find(result.thread_id) != m_thread_names.end())
				m_ostream << "\"tid\":\"" << m_thread_names[result.thread_id] << "\",";
			else
				m_ostream << "\"tid\":" << result.thread_id << ",";
			m_ostream << "\"ts\":" << result.time;
			m_ostream << "}";

			m_ostream.flush();
		}

		void write_header() {
			m_ostream << "{\"otherData\": {},\"traceEvents\":[";
			m_ostream.flush();
		}

		void write_footer() {
			m_ostream << "]}";
			m_ostream.flush();
		}

		void set_thread_name(size_t thread_id, std::string const& name) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_thread_names[thread_id] = name;
		}

	private:
		InstrumentationSession* m_current_session;
		std::ofstream m_ostream;
		int m_profile_count;
		std::string m_filepath;
		std::mutex m_mutex;
		std::unordered_map<size_t, std::string> m_thread_names;

	};

	struct InstrumentationTimer {
		InstrumentationTimer(char const* name)
			: m_name(name)
			, m_stopped(false) {
			m_start_timepoint = std::chrono::high_resolution_clock::now();
		}

		~InstrumentationTimer() {
			if (!m_stopped)
				stop();
		}

		void stop() {
			auto end_timepoint = std::chrono::high_resolution_clock::now();

			int64_t start = std::chrono::time_point_cast<std::chrono::microseconds>(m_start_timepoint).time_since_epoch().count();
			int64_t end = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch().count();

			size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			Instrumentor::get().write_profile({ m_name, start, end, thread_id });

			m_stopped = true;
		}

	private:
		char const* m_name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start_timepoint;
		bool m_stopped;
	};

	inline void report_counter(std::string const& name, int64_t value) {
		size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
		int64_t time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		Instrumentor::get().write_counter({ name, time, thread_id, value });
	}

	inline void report_instant(std::string const& name) {
		size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
		int64_t time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		Instrumentor::get().write_instant({ name, time, thread_id });
	}

	inline void report_flow(size_t id, std::string const& phase) {
		size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
		int64_t time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		Instrumentor::get().write_flow({ time, thread_id, id, phase });
	}

}
