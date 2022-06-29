// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_TIMER
#define __HOBO_VR_TIMER

#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <functional>
#include <chrono>
#include <ctime>

namespace hobovr {

using namespace std::chrono_literals;

class Timer {
public:
	Timer();
	~Timer();

	// no moving/coping this object
	Timer(const Timer&) = delete;
	Timer(Timer&&) = delete;
	Timer& operator= (Timer) = delete;
	Timer& operator= (Timer&&) = delete;

	template <class Rep, class Duration>
	inline void registerTimer(
		const std::chrono::duration<Rep, Duration>& delay,
		std::function<void(void)> func
	) {
		if (delay <= 0s || func == nullptr)
			return; // do nothing in case of bad args

		std::lock_guard lk(m_mutex);

		// calculate next callback wakeup time and register it
		auto next_wakeup = std::chrono::high_resolution_clock::now().time_since_epoch() + delay;
		m_timers.push_back({[=]() -> std::chrono::nanoseconds {func(); return delay;}, next_wakeup});
	}

private:
	void internal_thread();

	std::unique_ptr<std::thread> m_thread;

	std::mutex m_mutex;
	std::atomic<bool> m_alive{true};

	std::vector<std::pair<std::function<std::chrono::nanoseconds(void)>, std::chrono::nanoseconds>> m_timers;
};

} // namespace hobovr

#endif // #ifndef __HOBO_VR_TIMER