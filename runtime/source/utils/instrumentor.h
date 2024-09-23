#pragma once

#include "core/core.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <thread>

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

    struct InstrumentationSession {
        std::string name;
    };

    struct Instrumentor {
        Instrumentor() : m_current_session(nullptr), m_profile_count(0) {}

        void begin_session(std::string const& name, std::string const& filepath = "profile.json") {
            m_ostream.open(filepath);
            write_header();
            m_current_session = new InstrumentationSession{ name };
            m_profile_count = 0;
        }

        void end_session() {
            write_footer();
            m_ostream.close();
            delete m_current_session;
            m_current_session = nullptr;
            m_profile_count = 0;
        }

        void write_profile(ProfileResult const& result) {
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
            m_ostream << "\"tid\":" << result.thread_id << ",";
            m_ostream << "\"ts\":" << result.start;
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

    private:
        InstrumentationSession* m_current_session;
        std::ofstream m_ostream;
        int m_profile_count;
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
            g_runtime_context.m_instrumentor->write_profile({ m_name, start, end, thread_id });

            m_stopped = true;
        }

    private:
        char const* m_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start_timepoint;
        bool m_stopped;
    };

}
