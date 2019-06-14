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

namespace ev3_event_broker {
class TachoMotor {
private:
	int m_fd_command;
	int m_fd_position;
	int m_fd_duty_cycle;
	int m_fd_address;

public:
	explicit TachoMotor(const char* path);
	TachoMotor(const TachoMotor &) = delete;
	TachoMotor(TachoMotor &&other);

	~TachoMotor();

	void reset();
	int get_position() const;
	void set_duty_cycle(int duty_cycle);
	size_t name(char *buf, size_t buf_len) const;
};
}  // namespace ev3_event_broker
