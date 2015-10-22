#!/bin/sh
#
# Issue an ASignal signal on the bus
# dbus-send --bus=tcp:host=10.0.0.1,port=12330 --type=signal / com.konsulko.ucdbus.Service.ASignal

dbus-send					\
		--session			\
		--type=signal			\
		/				\
		com.konsulko.ucdbus.Service.ASignal

