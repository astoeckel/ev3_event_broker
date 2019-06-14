#!/usr/bin/env python3

#  EV3 Event Broker -- Talk to Lego Robots using UDP
#  Copyright (C) 2019  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

import json
import logging
import numpy as np
import subprocess
import sys
import threading
import time

logger = logging.getLogger('ev3_nengo')

ON_POSIX = 'posix' in sys.builtin_module_names


class Ev3BrokerClientWrapper:

    _insts = {}

    @staticmethod
    def get_empty_source():
        return {
            "source_hash": None,
            "positions": {},
            "duty_cycles": {},
        }

    def get_source_for_message(self, msg):
        # Make sure all the important message parts are here
        if not all(s in msg for s in (
                "source_name", "source_hash", "ip", "port")):
            return None

        # Fetch the source entry
        source_name = msg["source_name"]
        if (not source_name in self.sources):
            self.sources[source_name] = self.get_empty_source()
        dev = self.sources[source_name]
        if dev["source_hash"] != msg["source_hash"]:
            dev["source_hash"] = msg["source_hash"]
            dev["ip"] = msg["ip"]
            dev["port"] = msg["port"]

        return dev

    @staticmethod
    def parse_subprocess_output(self, stdout):
        for line in iter(stdout.readline, b''):
            try:
                msg = json.loads(str(line, 'utf-8'))
                if not "type" in msg:
                    continue
                type_ = msg["type"]
                if type_ == "error":
                    logger.error(msg["what"])
                    continue

                source = self.get_source_for_message(msg)
                if source is None:
                    continue

                type_ = None if not "type" in msg else msg["type"]
                if type_ == "position":
                    source["positions"][msg["device"]] = msg["position"]
                if not "type" in msg:
                    continue

            except json.JSONDecodeError:
                pass
        stdout.close()

    @classmethod
    def inst(class_, exe=None, port=None, name=None):
        key = (exe, port, name)
        if not key in class_._insts:
            class_._insts[key] = Ev3BrokerClientWrapper(
                exe=exe, port=port, name=name)
        return class_._insts[key]

    def __init__(self, exe=None, port=None, name=None):
        # Per default, search the event broker executable in PATH
        if exe is None:
            exe = 'ev3_broker_client'
        if port is None:
            port = 4721
        if name is None:
            name = 'nengo'

        # Map containing information about all devices that have been seen on
        # the network
        self.sources = {}

        # Open the executable
        self.process = subprocess.Popen(
            [exe, '-p', str(port), '-n', name],
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.STDOUT, bufsize=1, close_fds=ON_POSIX)
        self.thread = threading.Thread(
            target=Ev3BrokerClientWrapper.parse_subprocess_output, args=(self, self.process.stdout))
        self.thread.start()

    def set_duty_cycle(self, target, device, duty_cycle):
        # Cancel if the subprocess is no longer open
        if self.process is None:
            return False

        # Make sure the duty cycle is an integer in the valid range
        duty_cycle = int(max(min(round(duty_cycle), 100), -100))

        # Create an empty source if the given target does not exist
        if not target in self.sources:
            self.sources[target] = self.get_empty_source()
        source = self.sources[target]

        # Only send a duty cycle update in case there is a change in the duty
        # cycle
        if not device in source["duty_cycles"]:
            source["duty_cycles"][device] = None
        if duty_cycle != source["duty_cycles"][device]:
            source["duty_cycles"][device] = duty_cycle
            if not(("ip" in source) and ("port" in source)):
                return False

            # Assemble the message
            msg = {
                "ip": source["ip"],
                "port": source["port"],
                "type": "set_duty_cycle",
                "device": device,
                "duty_cycle": duty_cycle,
            }
            self.process.stdin.write((json.dumps(msg) + '\n').encode('utf-8'))
            self.process.stdin.flush()

    def reset(self, target=None):
        # Cancel if the subprocess is no longer open
        if self.process is None:
            return False

        for source_name, source in self.sources.items():
            if (target is None) or (target == source_name):
                if not (("ip" in source) and ("port" in source)):
                    continue
                msg = {
                    "ip": source["ip"],
                    "port": source["port"],
                    "type": "reset",
                }
                for i in range(10):
                    self.process.stdin.write((json.dumps(msg) + '\n').encode('utf-8'))
                    self.process.stdin.flush()

    def close(self):
        if not self.process is None:
            self.reset()
            self.process.stdin.close()
            self.process.wait()
            self.process = None

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self.close()

def make_node_fun(target, exe=None, port=None, name=None):
    # Fetch a new instance of the client wrapper
    inst = Ev3BrokerClientWrapper.inst(exe=exe, port=port, name=name)

    # Register this source
    if not target in inst.sources:
        inst.sources[target] = inst.get_empty_source()
    source = inst.sources[target]

    def node_fun(t, x):
        # Control the motors
        inst.set_duty_cycle(target, "motor_outA", x[0] * 100.0)
        inst.set_duty_cycle(target, "motor_outB", x[1] * 100.0)
        inst.set_duty_cycle(target, "motor_outC", x[2] * 100.0)
        inst.set_duty_cycle(target, "motor_outD", x[3] * 100.0)

        # Return the positions
        res = np.zeros(4)
        for i, dev in enumerate(["motor_outA", "motor_outB", "motor_outC", "motor_outD"]):
            if dev in source["positions"]:
                angle = np.pi * source["positions"][dev] / 180.0
                x, y = np.sin(angle), np.cos(angle)
                res[i] = np.arctan2(x, y) / np.pi
        return res

    setattr(node_fun, 'ev3_wrapper', inst)
    return node_fun

if __name__ == "__main__":
    import time
    logging.basicConfig()

    node_fun = make_node_fun('ev3_1', exe='./ev3_broker_client')
    for i in range(-10, 10):
        node_fun(0, np.array((1, -3, 2, -2)) * i / 100)
        time.sleep(0.1)
    node_fun.ev3_wrapper.close()
