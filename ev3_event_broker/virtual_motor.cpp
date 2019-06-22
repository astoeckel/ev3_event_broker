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

#ifdef VIRTUAL_MOTORS

#include <algorithm>
#include <cmath>

#include <time.h>

#include <ev3_event_broker/virtual_motor.hpp>

namespace ev3_event_broker {

static constexpr double MOTOR_TAU = 100.0e-3;
static constexpr double MOTOR_MAX_RPM = 240.0;

static double get_timestamp()
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return double(tp.tv_sec) + double(tp.tv_nsec) * 1e-9;
}

VirtualMotor::VirtualMotor(const char *path)
    : TachoMotor(path),
      m_x0(0.0),
      m_v0(0.0),
      m_t0(0.0),
      m_vtar(0.0),
      m_pos_offs(0.0)
{
}

VirtualMotor::~VirtualMotor() {}

double VirtualMotor::get_precise_velocity(double t1) const
{
	const double t = t1 - m_t0;
	return std::exp(-t / MOTOR_TAU) * (m_v0 - m_vtar) + m_vtar;
}

double VirtualMotor::get_precise_position(double t1) const
{
	const double t = t1 - m_t0;
	return MOTOR_TAU * (m_v0 - std::exp(-t / MOTOR_TAU) * (m_v0 - m_vtar)) +
	       m_vtar * (t - MOTOR_TAU) + m_x0;
}

void VirtualMotor::reset()
{
	set_duty_cycle(0);
	m_pos_offs = get_precise_position(get_timestamp());
}

int VirtualMotor::get_position() const
{
	return int((get_precise_position(get_timestamp()) - m_pos_offs) * 360.0);
}

void VirtualMotor::set_duty_cycle(int duty_cycle)
{
	// Get the current position and RPM
	const double t = get_timestamp();
	const double v0 = get_precise_velocity(t);
	const double x0 = get_precise_position(t);
	m_v0 = v0, m_x0 = x0, m_t0 = t;

	// Compute the target RPM
	duty_cycle = std::max(std::min(duty_cycle, 100), -100);
	m_vtar = (double(duty_cycle) / 100.0) * (MOTOR_MAX_RPM / 60.0);
}
}  // namespace ev3_event_broker

#endif
