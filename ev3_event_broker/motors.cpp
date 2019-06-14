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

#include <cstring>

#include <dirent.h>

#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/motors.hpp>

namespace ev3_event_broker {
Motors::Motors() { rescan(); }

void Motors::rescan() {
	const char motor_root_dir[] = "/sys/class/tacho-motor";
	const size_t motor_root_dir_len = sizeof(motor_root_dir) - 1;
	char buf[1024];
	char name_buf[1024];
	strncpy(buf, motor_root_dir, sizeof(buf));

	// Remove all motors from the list that no longer can be probed
	for (auto it = m_motors.begin(); it != m_motors.end();) {
		try {
			it->second.name(name_buf, sizeof(name_buf));
			it++;
		} catch (std::system_error &) {
			it = m_motors.erase(it);
		}
	}

	// Iterate over the motor root directory
	DIR *d;
	struct dirent *dir;
	d = opendir(motor_root_dir);
	if (d) {
		while ((dir = readdir(d)) != nullptr) {
			// For each file, try to create a motor instance. If this succeeds,
			// get the motor name and add it to the motor map
			try {
				// Create the absolute motor path
				snprintf(buf + motor_root_dir_len,
				         sizeof(buf) - motor_root_dir_len, "/%s", dir->d_name);

				// Create the motor instance
				TachoMotor motor(buf);

				// Fetch the motor instance path. In case it already exists in
				// the map, do nothing. Otherwise reset the motor and add it
				// to the mao
				size_t name_len = motor.name(name_buf, sizeof(name_buf));
				std::string name(name_buf, name_len);
				if (m_motors.find(name) == m_motors.end()) {
					motor.reset();
					m_motors.emplace(
						std::string(name_buf, name_len), std::move(motor));
				}
			} catch (std::system_error &) {
				// Ignore failures at this point
			}
		}
		closedir(d);
	}
}
}  // namespace ev3_event_broker
