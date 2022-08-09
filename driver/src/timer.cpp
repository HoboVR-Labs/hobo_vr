// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "timer.h"

namespace hobovr {

///////////////////////////////////////////////////////////////////////////////

Timer::Timer() {
	m_thread = std::make_unique<std::thread>(
		std::bind(&Timer::internal_thread, this)
	);
}

///////////////////////////////////////////////////////////////////////////////

Timer::~Timer() {
	m_alive.store(false);
	if (m_thread->joinable())
		m_thread->join();
}

///////////////////////////////////////////////////////////////////////////////

void Timer::internal_thread() {
	while(m_alive.load()) {
		// calculate next sleep duration
		auto next_wakeup = std::chrono::high_resolution_clock::now().time_since_epoch() + 3s;

		{ // locked context, to not lock while in sleep
		std::lock_guard lk(m_mutex);

		for (auto& i : m_timers) {
			if (i.second < next_wakeup)
				next_wakeup = i.second;
		}
		}

		// sleep
		std::this_thread::sleep_until(
			std::chrono::time_point<std::chrono::high_resolution_clock>(next_wakeup)
		);
		auto now = std::chrono::high_resolution_clock::now().time_since_epoch();

		{ // locked context, to not lock while in sleep
		std::lock_guard lk(m_mutex);
		// process callbacks
		for (auto& i : m_timers) {
			if (i.second <= now)
				i.second = now + i.first();
		}
		}
	}
}


} // namespace hobovr