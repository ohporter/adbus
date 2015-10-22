#!/bin/sh
#
# Issue a Quit method call to the com.konsulko.ucdbus.Simple service
# dbus-send --bus=tcp:host=10.0.0.1,port=12330 --dest=com.konsulko.ucdbus.Simple --type=method_call --print-reply / com.konsulko.ucdbus.Simple.Quit

dbus-send						\
		--session				\
		--dest=com.konsulko.ucdbus.Simple	\
		--type=method_call			\
		--print-reply				\
		/					\
		com.konsulko.ucdbus.Simple.Quit

