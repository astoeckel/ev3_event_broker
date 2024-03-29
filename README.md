# EV3 EVENT BROKER
**Communicate with LEGO® EV3 Bricks using UDP**

This repository contains two programs `ev3_broker_server` and `ev3_broker_client`. These programs facilitate high-speed communication with LEGO® EV3 bricks running the [`ev3dev`](https://www.ev3dev.org/) operating system. Furthermore, this repository contains a Python module that can be used to integrate EV3 bricks into [Nengo](https://nengo.ai/) networks.

The programs in this repository are designed to be relatively fast ‒ for example, the server program `ev3_broker_server` running on the EV3 hardware will never perform any heap allocations after startup, and the use of UDP as an underlying transport protocol minimizes overhead. This software has been successfully tested in PID control loops (running at about 100 Hz), where the controller resides on a host computer.

**Note:** As of now, the software does not contain support for devices other than the `tacho-motor`.

## Features

* Software runs on Linux, FreeBSD, and MacOS, as well as Windows when using WSL
* Lightweight, connection-less UDP-based message bus
* Motor auto-discovery and hot-plug capability
* Nengo integration
* Allows to read tacho-motor positions and to set the motor PWM duty-cycle (roughly proportional to the current/torque)

## Overview
*EV3 Event Broker* is based on a simple UDP-based message protocol (see below for a description of the format). Each EV3 device in the network is assigned a unique name which can be used to identify their IP address on the network.

`ev3_broker_server` runs on the LEGO® brick and broadcasts sensor data as UDP packages. At the same time it waits for incoming UDP packages containing commands, such as setting the duty cycle of a motor. See below for a description of the binary message format.

`ev3_broker_client` receives messages from all bricks on the network and writes them to stdout. Data is encapsulated in JSON and can thus be easily processed in another application. Furthermore, `ev3_broker_client` waits on stdin for JSON-encapsulated strings containing commands. See below for a description of the JSON message format.

## Downloading and Compiling

### Compile for the host platform
To download and compile the prorams described above make sure to have a recent version of GNU make and a C++14 compliant compiler such as `g++` or `clang++` installed. Compilation was tested on Debian 9, Fedora 30, FreeBSD 12 and a recent version of MacOS.

To download and compile the software execute
```sh
git clone https://github.com/astoeckel/ev3_event_broker --recursive
cd ev3_event_broker
make
```
This should create the two executables `ev3_broker_client` and `ev3_broker_server`.

**Note:** Make sure to use `gmake` (GNU Make) instead of `make` on FreeBSD.

Execute
```sh
./ev3_broker_server --help
```
to get information on available command line arguments.

In case you forgot `--recursive` while cloning the repository, run
```
git submodule update --init
```
This will download the embedded copy of [`nlohmann::json`](https://github.com/nlohmann/json) used by `ev3_broker_client`; you do not need to download this subrepository if you only intend to use `ev3_broker_server`.

### Cross-compilation for ARM

On Debian, install the `g++-6-arm-linux-gnueabi` and `make` package. Then, set the environment variable `CXX` to the name of the cross-compiler (should be `arm-linux-gnueabi-g++`) and set the environment variable `LDFLAGS` to `-static` to build a static executable.
```sh
git clone https://github.com/astoeckel/ev3_event_broker --recursive
cd ev3_event_broker
CXX=arm-linux-gnueabi-g++ LDFLAGS=-static make
```
This will compile static executables `ev3_broker_server` and `ev3_broker_client` that can be copied and executed on the EV3 brick.

**Note:** Make sure to execute the above commands in a fresh clone of the repository or execute `make clean` before setting the `CXX` and `LDFLAGS` environment variable; otherwise `make` will not re-compile the executables.

**Note:** Using Debian for cross-compilation is recommended. You can ‒ for example ‒ perform the above in a Debian VM or a Debian Docker container.

### Use virtual motors

Specify the `VIRTUAL_MOTORS` pre-processor macro to enable virtual motors in `ev3_broker_server`. These virtual motors can be used for testing purposes without having to use the actual hardware. Execute
```sh
CPPFLAGS=-DVIRTUAL_MOTORS make
```
Next, run the `make_virtual_motor_dirs.sh` script. This will create a directory structure that looks similar to the structure found in `/sys/class/tacho-motor` on the EV3 brick. `ev3_broker_server` will read this directory structure and creates motors accordingly.

**Note:** Make sure to execute the above commands in a fresh clone of the repository or execute `make clean` before setting the `CPPFLAGS` environment variable; otherwise `make` will not re-compile the executables.

## Nengo integration

The following example shows how to safely integrate *EV3 Event Broker* into a Nengo GUI script. This script will create a node that has four inputs (corresponding to the torques applied to the four possible motors, normalised to -1.0 to 1.0) and four outputs (normalised to 1.0 = 360°).

```py
# Add the ev3_broker_client directory to the Python path, determine the name
# of the client executable. Assumes the "ev3_event_broker" repository is a
# sub-directory of the current directory.
import sys, os
broker_dir = os.path.join(os.path.dirname(__file__), "ev3_event_broker")
broker_exe = os.path.join(broker_dir, "ev3_broker_client")
sys.path.append(os.path.join(broker_dir, "python"))
import ev3_nengo

# Listen to the server device with the name 'EV3', create a
# function that can be passed to a nengo node
node_fun = ev3_nengo.make_node_fun('EV3', exe=broker_exe)

# Only activate communication with the client right away if we're not in the
# GUI
active = [__name__ == '__main__']
def node_fun_wrapper(t, x):
    if active[0]:
        return node_fun(t, x)
    else:
        return np.zeros(4)

# Create the nengo network
import nengo
with nengo.Network() as model:
    node_env = nengo.Node(node_fun_wrapper, size_in=4, label="env")

# Hook into nengo GUI state change callbacks
def on_pause(sim):
    print("Pause")
    active[0] = False
    node_fun.reset()

def on_start(sim):
    print("Start")
    node_fun.reset()
    active[0] = True
```

## JSON message format

`ev3_broker_client` uses a convenient JSON-based message format. Each line printed to `stdout` corresponds to a message. Similarly, when piping a command into `ev3_broker_client`, each message must be written as an individual line.

**Note:** In the following examples the messages are printed over several lines for better
readability.

### Motor position broadcast (`server --> client`)
Sent in 10ms intervals for each motor attached to the EV3 brick.
```js
{
	"type": "position",
	"ip": [A,B,C,D], // IPv4 address A.B.C.D of the source device
	"port": 4721, // Port on which the message was received
	"source_name": "EV3", // Server name
	"source_hash": "kyv5mpZ8", // Random string identifying the server
	"device": "motor_outX", // Motor on port X
	"position": 0, // Motor position in degrees
	"seq": 0 // Message sequence number
}
```

**Note:** The position will be reset to zero whenever a motor is reset or unplugged/plugged back in.

### Heartbeat broadcast (`server --> client`)
Sent in 250ms intervals from each device on the network.
```js
{
	"type": "heartbeat",
	"ip": [A, B, C, D], // IPv4 address A.B.C.D of the source device
	"port": 4721, // Port on which the message was received
	"source_name": "EV3", // Server name
	"source_hash": "kyv5mpZ8", // Random string identifying the server
	"seq": 0 // Message sequence number
}
```

### Set duty cycle (`client --> server`)
Command to adjust the PWM duty cycle of a target motor.
```js
{
	"type": "set_duty_cycle",
	"ip": [A, B, C, D], // IPv4 address A.B.C.D of the target device
	"port": 4721, // Target port
	"device": "motor_outX", // Which motor to control
	"duty_cycle": 0 // Motor duty cycle between -100 and 100
}
```

### Reset (`client --> server`)
Resets all motors attached to the target device.
```js
{
	"type": "reset",
	"ip": [A, B, C, D], // IPv4 address A.B.C.D of the target device
	"port": 4721, // Target IPv4 port
}
```

## Binary message format

All integers are serialized as **big-endian**. All strings are fixed size; if the string is shorter than the indicated number of bytes, the remaining space is filled with zeros. A single message consists of *n* sub-messages, as indicated in the below message header format. Each sub-message starts with a single `type` byte.

### Message header

The message header is used in both communication directions. Note that the `source name` and `source hash` always refer to the sender of the message; i.e., in `client --> server` messages the source name and source hash could be used by the server to identify the client. In particular, both `ev3_broker_client` and `ev3_broker_server` will discard all messages in which `source_name` and `source_hash` match their own name and hash. This is necessary because of UDP broadcasts being received by the sender itself.

When assembling these messages, you must ensure that the total packet length is never larger than the MTU (the C++ code limits the packet size to 1280 bytes).

```
Sync word   |   4 Bytes | 0xCAA29C3A
Source name |  16 Bytes | string
Source hash |   8 Bytes | string
Sequence    |   4 Bytes | unsigned int
#Messages   |   1 Byte  | unsigned int
```

The `#Messages` field indicates the number of messages following the message header.

### Motor position broadcast (`server --> client`)
```
Type       |    1 Byte  | 0x01
Device     |    8 Bytes | string
Position   |    4 Bytes | unsigned int
```

### Set duty cycle (`client --> server`)
```
Type       |    1 Byte  | 0x02
Device     |   16 Bytes | string
Duty cycle |    4 Bytes | signed int
```

### Heartbeat broadcast (`server --> client`)
```
Type       |    1 Bytes | 0x03
```

### Reset (`client --> server`)
```
Type       |    1 Bytes | 0xFF
```

## License

```
EV3 Event Broker -- Talk to Lego Robots using UDP
Copyright (C) 2019  Andreas Stöckel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
