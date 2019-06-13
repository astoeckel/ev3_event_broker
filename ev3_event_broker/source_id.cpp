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

#include <cstddef>
#include <random>

#include <ev3_event_broker/source_id.hpp>

namespace ev3_event_broker {

static std::random_device random_device;

static void generate_random_string(char *tar, size_t n) {
	static const char alphabet[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const size_t len = sizeof(alphabet) - 1;
	std::default_random_engine generator(random_device());
	std::uniform_int_distribution<int> distribution(0, len - 1);
	for (size_t i = 0; i < n; i++) {
		tar[i] = alphabet[distribution(generator)];
	}
	tar[n] = 0;
}

SourceId::SourceId(const char *name) : m_name(name) {
	generate_random_string(m_hash, sizeof(m_hash) - 1);
}

}
