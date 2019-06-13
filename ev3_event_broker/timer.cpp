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

#include <cstdint>

#include <unistd.h>
#include <sys/timerfd.h>

#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/timer.hpp>

namespace ev3_event_broker {

Timer::Timer(int interval_ms) {
	struct itimerspec ts;
	ts.it_interval.tv_sec = interval_ms / 1000L;
	ts.it_interval.tv_nsec = long(interval_ms % 1000L) * 1000L * 1000L;
	ts.it_value.tv_sec = ts.it_interval.tv_sec;
	ts.it_value.tv_nsec = ts.it_interval.tv_nsec;

	m_fd = err(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
	err(timerfd_settime(m_fd, 0, &ts, nullptr));
}

Timer::~Timer() { close(m_fd); }

void Timer::consume_event() {
	uint64_t buf;
	err(read(m_fd, &buf, 8));
}

}  // namespace ev3_event_broker
