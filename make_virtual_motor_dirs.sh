#!/bin/bash

function make_motor_dir {
	mkdir -p "$1"
	touch "$1/command"
	echo 0 > "$1/position"
	touch "$1/duty_cycle_sp"
	echo "running" > "$1/state"
	echo "$2" > "$1/address"
}

make_motor_dir motors/motor0 outA
make_motor_dir motors/motor1 outB
make_motor_dir motors/motor2 outC
make_motor_dir motors/motor3 outD


