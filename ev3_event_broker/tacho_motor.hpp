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
	int m_fd_state;
	char m_name[17];

	void read_name(const char *path);

public:
	explicit TachoMotor(const char *path);
	TachoMotor(const TachoMotor &) = delete;
	TachoMotor &operator=(const TachoMotor &) = delete;

	TachoMotor(TachoMotor &&other);
	TachoMotor &operator=(TachoMotor &&);

	~TachoMotor();

	void reset();

	bool good() const;

	int get_position() const;
	void set_duty_cycle(int duty_cycle);

	const char *name() const { return m_name; }
};
}  // namespace ev3_event_broker
