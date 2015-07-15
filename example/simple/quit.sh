#!/bin/sh
#
# Issue a Quit method call to the com.konsulko.ucdbus.Simple service

dbus-send						\
		--session				\
		--dest=com.konsulko.ucdbus.Simple	\
		--type=method_call			\
		--print-reply				\
		/					\
		com.konsulko.ucdbus.Simple.Quit
