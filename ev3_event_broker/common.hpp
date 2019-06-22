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

/**
 * @file common.hpp
 *
 * Contains some functions common to all the device drivers, such as opening a
 * control file belonging to a device.
 *
 * @author Andreas Stöckel
 */

#pragma once

#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ev3_event_broker/error.hpp>

namespace ev3_event_broker {

/**
 * Concatenates the two given strings and writes them to the target memory. Note
 * that at most tar_size - 1 characters will be copied to ensure that the target
 * string is properly zero-terminated.
 */
static inline void cat_cstr(const char *prefix, const char *suffix, char *tar,
                     size_t tar_size) {
	strncpy(tar, prefix, tar_size - 1);
	strncat(tar, suffix, tar_size - 1);
}

/**
 * Opens the device file "device_file" in the directory "device_path" and
 * returns the corresponding file descriptor.
 */
static int open_device_file(const char *device_path, const char *device_file,
                            int flags, mode_t mode = 0) {
	char filename[4096];
	cat_cstr(device_path, device_file, filename, sizeof(filename));
	return err(open(filename, flags, mode));
}

}  // namespace ev3_event_broker
