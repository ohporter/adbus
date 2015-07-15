#!/bin/sh
#
# Issue an ASignal signal on the bus

dbus-send					\
		--session			\
		--type=signal			\
		/				\
		com.konsulko.ucdbus.Service.ASignal
