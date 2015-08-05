dnl Check for debug build

dnl Usage: CHECK_ENABLE_DEBUG

AC_DEFUN([CHECK_ENABLE_DEBUG],
[
    AC_ARG_ENABLE([debug],
                  [AS_HELP_STRING([--enable-debug], [Enables assertions])],
                  [debug="$enableval"],
                  [debug="no"])

    AS_IF([test "x$debug" = "xyes"],
          [],
          [AC_DEFINE([NDEBUG], [], [Disables assertions])])

]) dnl end CHECK_ENABLE_DEBUG

