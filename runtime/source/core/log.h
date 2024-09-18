#pragma once

#include "core/core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace z1 {

    struct API Logger {
        Logger();

        inline std::shared_ptr<spdlog::logger>& get_core_logger() { return m_core_logger; }
        inline std::shared_ptr<spdlog::logger>& get_client_logger() { return m_client_logger; }

    private:
        std::shared_ptr<spdlog::logger> m_core_logger;
        std::shared_ptr<spdlog::logger> m_client_logger;
    };

}
