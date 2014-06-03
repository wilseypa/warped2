AC_INIT([warped],[0.1.0])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/Event.hpp])
AM_INIT_AUTOMAKE([1.11 foreign subdir-objects])
AM_SILENT_RULES([yes])
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

dnl Once MPI support is added, uncomment the next two lines
dnl m4_include([m4/check_lib_mpi.m4])
dnl CHECK_LIB_MPI

AC_CONFIG_FILES([deps/cereal/Makefile])
AC_CONFIG_FILES([deps/json/Makefile])
AC_CONFIG_FILES([deps/Makefile])
AC_CONFIG_FILES([deps/metis/GKlib/Makefile])
AC_CONFIG_FILES([deps/metis/libmetis/Makefile])
AC_CONFIG_FILES([deps/metis/Makefile])
AC_CONFIG_FILES([deps/tclap/Makefile])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_OUTPUT