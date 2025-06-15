#include "pch.h"
#include "core/timer.h"

namespace z1 {

	float Timer::fixed_update_delta = 0.016f;

	Timer::Timer() {
		m_prev_timepoint = m_start_timepoint = std::chrono::steady_clock::now();
	}

	void Timer::update() {
		auto now = std::chrono::steady_clock::now();
		auto delta_time_count = std::chrono::duration<float, std::micro>(now - m_prev_timepoint).count();
		m_delta_time = 1e-6f * delta_time_count;
		m_prev_timepoint = now;

		m_fixed_delta_time += m_delta_time;
		if (m_fixed_delta_time > fixed_update_delta) {
			m_should_fixed_update = true;
			m_fixed_delta_time -= fixed_update_delta;
		}
		else {
			m_should_fixed_update = false;
		}
	}

	float Timer::get_delta_time() const {
		return m_delta_time;
	}

	float Timer::get_total_time() const {
		auto total_time_count = std::chrono::duration<float, std::micro>(m_prev_timepoint - m_start_timepoint).count();
		return 1e-6f * total_time_count;
	}

	bool Timer::should_fixed_update() const {
		return m_should_fixed_update;
	}

}
