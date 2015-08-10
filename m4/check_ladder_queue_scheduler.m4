dnl Check for debug build

dnl Usage: CHECK_LADDER_QUEUE_SCHEDULER

AC_DEFUN([CHECK_LADDER_QUEUE_SCHEDULER],
[
    AC_ARG_WITH([ladder-queue-scheduler],
                [AS_HELP_STRING([--with-ladder-queue-scheduler], [Use ladder queue as scheduler])],
                [want_ladderq="$withval"],
                [want_ladderq="no"])

    AS_IF([test "x$want_ladderq" = "xyes"],
          [AC_DEFINE([LADDER_QUEUE_SCHEDULER], [], [Use ladder queue as scheduler])],
          [])

]) dnl end CHECK_LADDER_QUEUE_SCHEDULER

