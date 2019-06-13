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

#include <cstdio>
#include <string>

#include <unistd.h>

#include <ev3_event_broker/event_loop.hpp>
#include <ev3_event_broker/marshaller.hpp>
#include <ev3_event_broker/socket.hpp>
#include <ev3_event_broker/tacho_motor.hpp>
#include <ev3_event_broker/timer.hpp>

using namespace ev3_event_broker;

int main(int argc, char *argv[])
{
	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address broadcast_address(192, 168, 180, 255, port);
	socket::UDP sock(listen_address);

	const char *source_name = "EV3_1";
	const char *source_hash = "UAV6nOKM";

	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    /*		Message msg{buf, buf_size};*/
		    write(STDOUT_FILENO, buf, buf_size);
		    /*		sock.send(broadcast_address, msg);*/
		    return true;
	    },
	    source_name, source_hash);

	marshaller.write_position_sensor("motor_A", 3911);
	marshaller.write_position_sensor("motor_B", 778);
	marshaller.flush();

	marshaller.write_position_sensor("motor_C", 3911);
	marshaller.write_position_sensor("motor_D", 778);
	marshaller.flush();

	/*	Timer timer(1000);
	    // TachoMotor motor("/sys/class/tacho-motor/motor0");
	    //TachoMotor motor("/sys/class/tacho_motor/motor0");

	    EventLoop()
	        .register_event(
	            timer,
	            [&]() -> bool {
	                // Mark the timer event as handled
	                timer.consume_event();

	                // Broadcast the motor positions
	                uint8_t buf[1400];
	                size_t size = snprintf(
	                    reinterpret_cast<char *>(buf), sizeof(buf),
	                    "{\"src\":\"%s\",\"dev\":\"motor_D\",\"pos\":%d}\n",
	                    device_id, motor.get_position());

	                Message msg{buf, size};
	                sock.send(broadcast_address, msg);
	                return true;
	            })
	        .register_event(sock,
	                        [&]() -> bool {
	                            Address addr;
	                            Message msg;
	                            if (!sock.recv(addr, msg)) {
	                                return false;  // Socket was closed
	                            }
	                            return true;
	                        })
	        .run();*/

	return 0;
}





