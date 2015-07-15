AC_INIT([libucdbus], [0.1.0], [mporter@konsulko.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_AR
AC_CONFIG_MACRO_DIR([m4])
AC_DEFINE([_GNU_SOURCE], [], [Use GNU extensions])
LT_INIT
AC_CONFIG_FILES([Makefile src/Makefile example/simple/Makefile libucdbus.pc])
DX_INIT_DOXYGEN([$PACKAGE_NAME],[doxygen.cfg])
AC_OUTPUT
