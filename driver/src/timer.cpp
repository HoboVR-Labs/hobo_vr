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

template<class Rep, class Duration>
void Timer::registerTimer(
	const std::chrono::duration<Rep, Duration>& delay,
	std::function<void(void)> func
) {
	// register a new timer
	if (delay < m_delay.load())
		m_delay.store(delay);

	if (delay <= 0s || func == nullptr)
		return; // do nothing in case of bad args

	std::unique_lock<std::mutex> lk(m_mutex);
	m_timers.push_back({[=]() -> std::chrono::nanoseconds {func(); return delay;}, delay});
}

///////////////////////////////////////////////////////////////////////////////

template<class Rep, class Duration>
void Timer::setGlobalDelay(const std::chrono::duration<Rep, Duration>& delay) {
	m_delay.store(delay);
}

///////////////////////////////////////////////////////////////////////////////

std::chrono::nanoseconds Timer::getGlobalDelay() {
	return m_delay.load();
}

///////////////////////////////////////////////////////////////////////////////

void Timer::internal_thread() {
	std::chrono::nanoseconds exec_time = 0s;

	while(m_alive.load()) {
		std::this_thread::sleep_for(
			m_delay.load()
		);

		{ // locked context
			std::unique_lock<std::mutex> lk(m_mutex);

			// update callback exec times
			if (exec_time > 0s) {
				for (auto& i : m_timers) {
					i.second -= exec_time;
				}
				exec_time = 0s;
			}

			// process timer callbacks
			for (auto& i : m_timers) {
				i.second -= m_delay.load();

				if (i.second <= 0s){
					auto t1 = std::chrono::steady_clock::now();
					i.second = i.first();
					auto t2 = std::chrono::steady_clock::now();
					exec_time += (t2 - t1);
				}
			}

		}
	}
}

} // namespace hobovr