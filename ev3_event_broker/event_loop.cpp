/**
 *  EV3 Event Broker -- Talk to Lego Robots using UDP
 *  Copyright (C) 2019  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <vector>

#include <poll.h>
#include <unistd.h>

#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/event_loop.hpp>

namespace ev3_event_broker {

/******************************************************************************
 * Class EventLoop::Impl                                                      *
 ******************************************************************************/

class EventLoop::Impl {
private:
	static int64_t now()
	{
		const auto t = std::chrono::steady_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
	}

	struct Timer {
		Callback cback;
		int32_t interval;
		int64_t next_time;

		Timer(const Callback &cback, int32_t interval, int64_t next_time)
		    : cback(cback), interval(interval), next_time(next_time)
		{
		}
	};

	std::vector<Callback> m_cbacks;
	std::vector<struct pollfd> m_pollfds;

	std::vector<Timer> m_timers;

	int compute_timeout()
	{
		int64_t t0 = now();
		int64_t timeout = INT_MAX;
		for (const auto &timer : m_timers) {
			timeout = std::min(timeout, timer.next_time - t0);
		}
		return timeout;
	}

public:
	void register_event_fd(int fd, const EventLoop::Callback &cback)
	{
		m_cbacks.push_back(cback);
		m_pollfds.emplace_back(pollfd{fd, POLLIN, 0});
	}

	void register_timer(int interval_ms, const EventLoop::Callback &cback)
	{
		m_timers.emplace_back(cback, interval_ms, now() + interval_ms);
	}

	void run()
	{
		while (true) {
			// Compute the time until the next timeout event
			int timeout = compute_timeout();

			// If there is time to wait, wait for incoming events
			if (timeout > 0) {
				int res;
				res = poll(m_pollfds.data(), m_pollfds.size(), timeout);
				if (res >= 0) {
					for (size_t i = 0; i < m_pollfds.size(); i++) {
						struct pollfd &fd = m_pollfds[i];
						if (fd.revents) {
							fd.revents = 0;
							if (!m_cbacks[i]()) {
								return;
							}
						}
					}
				}
				else if (res < 0 && errno == EINTR) {
					continue;
				}
				else {
					throw std::system_error(errno, std::system_category());
				}
			}

			// Execute timers
			int64_t t = now();
			for (Timer &timer : m_timers) {
				if (t >= timer.next_time) {
					timer.next_time = t + timer.interval;
					if (!timer.cback()) {
						return;
					}
				}
			}
		}
	}
};

/******************************************************************************
 * Class EventLoop                                                            *
 ******************************************************************************/

EventLoop::EventLoop() : m_impl(new Impl()) {}

EventLoop::~EventLoop()
{
	// Do nothing here, implicitly delete the object
}

EventLoop &EventLoop::register_event_fd(int fd, const Callback &cback)
{
	m_impl->register_event_fd(fd, cback);
	return *this;
}

EventLoop &EventLoop::register_timer(int interval_ms, const Callback &cback)
{
	m_impl->register_timer(interval_ms, cback);
	return *this;
}

void EventLoop::run() { return m_impl->run(); }

}  // namespace ev3_event_broker
