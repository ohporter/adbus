include $(top_srcdir)/aminclude.am
SUBDIRS = src example/simple
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = doxygen.cfg
library_includedir=$(includedir)/ucdbus
library_include_HEADERS =	include/ucdbus/adbus.h	\
				include/ucdbus/adbus-iterator.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libucdbus.pc
