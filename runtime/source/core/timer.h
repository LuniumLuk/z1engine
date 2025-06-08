#pragma once

#include "core/core.h"
#include <chrono>

namespace z1 {

    struct API Timer {
        Timer();

        void update();
        float get_delta_time() const;
        float get_total_time() const;
        bool should_fixed_update() const;

        static float fixed_update_delta;

    private:
        std::chrono::steady_clock::time_point m_start_timepoint;
        std::chrono::steady_clock::time_point m_prev_timepoint;
        float m_delta_time = 0.0;
        float m_fixed_delta_time = 0.0;
        bool m_should_fixed_update = false;
    };

}
