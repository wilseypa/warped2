dnl Check for debug build

dnl Usage: CHECK_SORTED_LADDER_QUEUE

AC_DEFUN([CHECK_SORTED_LADDER_QUEUE],
[
    AC_ARG_WITH([sorted-ladder-queue],
                [AS_HELP_STRING([--with-sorted-ladder-queue], [Use sorted ladder queue as scheduler])],
                [want_sorted="$withval"],
                [want_sorted="no"])

    AS_IF([test "x$want_sorted" = "xyes"],
          [AC_DEFINE([SORTED_LADDER_QUEUE], [], [Use sorted ladder queue as scheduler])],
          [])

]) dnl end CHECK_SORTED_LADDER_QUEUE

