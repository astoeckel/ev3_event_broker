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

import subprocess
import sys
import threading
import json

ON_POSIX = 'posix' in sys.builtin_module_names


class Ev3BrokerClientWrapper:

    _inst = None

    @staticmethod
    def parse_subprocess_output(self, stdout):
        for line in iter(stdout.readline, b''):
            try:
                msg = json.loads(str(line, 'utf-8'))
            except json.JSONDecodeError:
                pass
        stdout.close()

    @classmethod
    def inst(class_, *args):
        if class_._inst is None:
            class_._inst = Ev3BrokerClientWrapper(*args)
        return class_._inst

    def __init__(self, exe=None, port=4721, name='nengo'):
        # Per default, search the event broker executable in PATH
        if exe is None:
            exe = 'ev3_broker_client'

        # Map containing information about all devices that have been seen on
        # the network
        self.devices = {}

        # Open the executable
        self.process = subprocess.Popen(
            [exe, '-p', str(port), '-n', name],
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=None, bufsize=1, close_fds=ON_POSIX)
        self.thread = threading.Thread(
            target=Ev3BrokerClientWrapper.parse_subprocess_output, args=(self, self.process.stdout))
        self.thread.start();


    def close(self):
        self.process.stdin.close()
        self.process.wait()
        return self.process.returncode

if __name__ == "__main__":
    import time
    ev3_wrapper = Ev3BrokerClientWrapper.inst('./ev3_broker_client')

    time.sleep(10)
    ev3_wrapper.close()

