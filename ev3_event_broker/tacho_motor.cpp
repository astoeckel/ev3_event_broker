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
#include <cstring>

#include <unistd.h>

#include <ev3_event_broker/common.hpp>
#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/tacho_motor.hpp>

namespace ev3_event_broker {

TachoMotor::TachoMotor(const char *path) {
	m_fd_command = open_device_file(path, "/command", O_WRONLY);
	m_fd_position = open_device_file(path, "/position", O_RDONLY);
	m_fd_duty_cycle = open_device_file(path, "/duty_cycle_sp", O_WRONLY);

	reset();
}

TachoMotor::~TachoMotor() {
	close(m_fd_duty_cycle);
	close(m_fd_position);
	close(m_fd_command);
}

void TachoMotor::reset() {
	{
		const char *str = "reset\n";
		err(pwrite(m_fd_command, str, strlen(str), 0));
	}
	{
		const char *str = "run-direct\n";
		err(pwrite(m_fd_command, str, strlen(str), 0));
	}
}

int TachoMotor::get_position() {
	char buf[16];
	size_t len = err(pread(m_fd_position, buf, sizeof(buf) - 1, 0));
	buf[len] = '\0';
	return atoi(buf);
}

void TachoMotor::set_duty_cycle(int duty_cycle) {
	if (duty_cycle > 100) {
		duty_cycle = 100;
	} else if (duty_cycle < -100) {
		duty_cycle = -100;
	}

	char buf[16];
	snprintf(buf, sizeof(buf), "%d\n", duty_cycle);
	err(pwrite(m_fd_duty_cycle, buf, strnlen(buf, sizeof(buf)), 0));
}
}  // namespace ev3_event_broker
