dnl Check for debug build

dnl Usage: CHECK_PARTIALLY_SORTED_LADDER_QUEUE

AC_DEFUN([CHECK_PARTIALLY_SORTED_LADDER_QUEUE],
[
    AC_ARG_WITH([partially-sorted-ladder-queue],
                [AS_HELP_STRING([--with-partially-sorted-ladder-queue], [Use partially sorted ladder queue as scheduler])],
                [want_partially_sorted="$withval"],
                [want_partially_sorted="no"])

    AS_IF([test "x$want_partially_sorted" = "xyes"],
          [AC_DEFINE([PARTIALLY_SORTED_LADDER_QUEUE], [], [Use partially sorted ladder queue as scheduler])],
          [])

]) dnl end CHECK_PARTIALLY_SORTED_LADDER_QUEUE

