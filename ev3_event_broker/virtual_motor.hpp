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

#ifdef VIRTUAL_MOTORS

#include <ev3_event_broker/tacho_motor.hpp>

namespace ev3_event_broker {
class VirtualMotor : public TachoMotor {
private:
	double m_x0, m_v0, m_t0, m_vtar, m_pos_offs;

	double get_precise_velocity(double t1) const;
	double get_precise_position(double t1) const;

public:
	explicit VirtualMotor(const char *path);
	~VirtualMotor() override;
	void reset() override;
	int get_position() const override;
	void set_duty_cycle(int duty_cycle) override;
};

}  // namespace ev3_event_broker

#endif

