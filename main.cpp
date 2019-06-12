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

#include <unistd.h>

#include <ev3_event_broker/socket.hpp>

using namespace ev3_event_broker;

int main(int argc, char *argv[]) {
	UDPSocket sock(Address(0, 0, 0, 0, 4721));
	Address addr;
	Message msg;
	while (sock.recv(addr, msg)) {
		write(STDOUT_FILENO, msg.buf(), msg.size());
		sock.send(addr, msg);
	}
}
