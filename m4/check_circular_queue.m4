dnl Check for debug build

dnl Usage: CHECK_CIRCULAR_QUEUE

AC_DEFUN([CHECK_CIRCULAR_QUEUE],
[
    AC_ARG_WITH([circular-queue],
                [AS_HELP_STRING([--with-circular-queue], [Use circular queue as scheduler])],
                [want_circularq="$withval"],
                [want_circularq="no"])

    AS_IF([test "x$want_circularq" = "xyes"],
          [AC_DEFINE([CIRCULAR_QUEUE], [], [Use circular queue as scheduler])],
          [])

]) dnl end CHECK_CIRCULAR_QUEUE

