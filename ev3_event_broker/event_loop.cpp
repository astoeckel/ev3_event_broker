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

#include <cstdlib>
#include <cstdint>
#include <vector>

#include <sys/epoll.h>
#include <unistd.h>

#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/event_loop.hpp>

/******************************************************************************
 * Class EventLoop::Impl                                                      *
 ******************************************************************************/

namespace ev3_event_broker {

class EventLoop::Impl {
private:
	std::vector<Callback> m_cbacks;
	std::vector<struct epoll_event> m_events;

	int m_epoll_fd;

public:
	Impl() { m_epoll_fd = err(epoll_create1(EPOLL_CLOEXEC)); }

	~Impl() {
		close(m_epoll_fd);
	}

	void register_event_fd(int fd, const EventLoop::Callback &cback) {
		// Get the index of the new event
		const size_t idx = m_cbacks.size();

		// Store the callback
		m_cbacks.push_back(cback);

		// Create an event
		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.u64 = uintptr_t(idx);

		// Register the event and store it in the list of events
		epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);

		// Reset
		event.events = 0;
		m_events.push_back(event);
	}

	void run() {
		while (true) {
			int count =
			    epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), -1);
			if (count >= 0) {
				for (struct epoll_event &event : m_events) {
					if (event.events) {
						const size_t idx = event.data.u64;
						event.events = 0;
						if (!m_cbacks[idx]()) {
							return;
						}
					}
				}
			} else if (count < 0 && errno == EINTR) {
				continue;
			} else {
				throw std::system_error(errno, std::system_category());
			}
		}
	}
};

/******************************************************************************
 * Class EventLoop                                                            *
 ******************************************************************************/

EventLoop::EventLoop() : m_impl(new Impl()) {
}

EventLoop::~EventLoop() {
	// Do nothing here, implicitly delete the object
}

EventLoop &EventLoop::register_event_fd(int fd, const Callback &cback) {
	m_impl->register_event_fd(fd, cback);
	return *this;
}

void EventLoop::run() { return m_impl->run(); }

}  // namespace ev3_event_broker
