AC_INIT([warped2],[0.1.0])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/Event.h])
AM_INIT_AUTOMAKE([1.11 silent-rules foreign subdir-objects])
AC_PREREQ(2.61)

AC_LANG([C++])

dnl Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL

LT_INIT

m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT