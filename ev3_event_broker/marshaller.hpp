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
#include <cstdint>
#include <functional>

namespace ev3_event_broker {

/**
 * Synchronisation word used to track the beginning of a message.
 */
static constexpr uint32_t SYNC = 0xCAA29C3AU;

/**
 * Message indicating the position of a motor.
 */
static constexpr uint8_t TYPE_POSITION_SENSOR = 0x01;

/**
 * Message indicating the desired motor duty cycle.
 */
static constexpr uint8_t TYPE_SET_DUTY_CYCLE = 0x02;

/**
 * Message indicating that a device is alive.
 */
static constexpr uint8_t TYPE_HEARTBEAT = 0x03;

/**
 * Message demanding the reset of all devices.
 */
static constexpr uint8_t TYPE_RESET = 0xFF;

/**
 * Maximum buffer size used by the marshaller. This should be approximately
 * equivalent to the MTU (preferrably smaller).
 */
static constexpr size_t MARSHALLER_BUF_SIZE = 1280;

/**
 * Number of characters in the source name.
 */
static constexpr size_t N_SOURCE_NAME_CHARS = 16;

/**
 * Number of charachters in the random source identifier.
 */
static constexpr size_t N_SOURCE_HASH_CHARS = 8;

/**
 * Number of characters used to identify devices.
 */
static constexpr size_t N_DEVICE_NAME_CHARS = 16;

static constexpr size_t HEADER_SIZE =
    N_SOURCE_NAME_CHARS + N_SOURCE_HASH_CHARS + 4;
static constexpr size_t POSITION_SENSOR_SIZE = 1 + N_DEVICE_NAME_CHARS + 4;
static constexpr size_t SET_DUTY_CYCLE_SIZE = 1 + N_DEVICE_NAME_CHARS + 4;
static constexpr size_t RESET_SIZE = 1;
static constexpr size_t HEARTBEAT_SIZE = 1;

class Marshaller {
public:
	using Callback = std::function<bool(const uint8_t *buf, size_t size)>;

private:
	Callback m_cback;
	uint8_t m_buf[MARSHALLER_BUF_SIZE];
	size_t m_buf_ptr;
	uint32_t m_sequence;
	uint8_t m_message_count;
	ptrdiff_t m_sequence_offs;
	ptrdiff_t m_header_offs;
	bool m_good;

	void flush_if_no_space(size_t size_required);

	uint8_t *initialze_msg(size_t size_required);
	Marshaller &finalize_msg(uint8_t *tar);

public:
	Marshaller(const Callback &cback, const char *source_name,
	           const char *source_hash);

	explicit operator bool() const { return m_good; }

	Marshaller &flush();

	Marshaller &write_position_sensor(const char *device_name,
	                                  int32_t position);
	Marshaller &write_set_duty_cycle(const char *device_name,
	                                 int32_t duty_cycle);
	Marshaller &write_heartbeat();
	Marshaller &write_reset();
};

class Demarshaller {
public:
	struct Header {
		char source_name[N_SOURCE_NAME_CHARS + 1];
		char source_hash[N_SOURCE_HASH_CHARS + 1];
		uint32_t sequence;
		uint8_t n_messages;
	};

	struct PositionSensor {
		char device_name[N_DEVICE_NAME_CHARS + 1];
		int32_t position;
	};

	struct SetDutyCycle {
		char device_name[N_DEVICE_NAME_CHARS + 1];
		int32_t duty_cycle;
	};

	struct Listener {
		Listener(){};

		virtual ~Listener(){};

		virtual bool filter(const Header &) { return true; }

		virtual void on_position_sensor(const Header &,
		                                const PositionSensor &){};

		virtual void on_set_duty_cycle(const Header &, const SetDutyCycle &){};

		virtual void on_heartbeat(const Header &) {};

		virtual void on_reset(const Header &){};
	};

private:
	uint32_t m_sync;
	Header m_header;
	uint8_t m_type;
	PositionSensor m_position_sensor;
	SetDutyCycle m_set_duty_cycle;

public:
	Demarshaller();

	void parse(Listener &listener, const uint8_t *buf, size_t buf_size);
};

}  // namespace ev3_event_broker
