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

#include <vector>
#include <string>
#include <memory>

#include <ev3_event_broker/motor.hpp>

namespace ev3_event_broker {
class Motors {
private:
	std::vector<std::unique_ptr<Motor>> m_motors;
public:
	Motors();

	void rescan();

	const std::vector<std::unique_ptr<Motor>> &motors() const { return m_motors; }
	std::vector<std::unique_ptr<Motor>> &motors() { return m_motors; }

	Motor *find(const char *name);
};
}
