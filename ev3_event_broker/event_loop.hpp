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

#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace ev3_event_broker {

class EventLoop {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	using Callback = std::function<bool()>;

	EventLoop();
	~EventLoop();

	EventLoop &register_event_fd(int fd, const Callback &cback);

	template <typename T>
	EventLoop &register_event(T &obj, const Callback &cback) {
		return register_event_fd(obj.fd(), cback);
	}

	EventLoop &register_timer(int interval_ms, const Callback &cback);

	void run();
};

}  // namespace ev3_event_broker
