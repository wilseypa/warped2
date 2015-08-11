dnl Check for debug build

dnl Usage: CHECK_PARTIALLY_UNSORTED_EVENT_SET

AC_DEFUN([CHECK_PARTIALLY_UNSORTED_EVENT_SET],
[
    AC_ARG_WITH([partially-unsorted-event-set],
                [AS_HELP_STRING([--with-partially-unsorted-event-set], [Use partially unsorted event set])],
                [want_partial_unsorted="$withval"],
                [want_partial_unsorted="no"])

    AS_IF([test "x$want_partial_unsorted" = "xyes"],
          [AC_DEFINE([PARTIALLY_UNSORTED_EVENT_SET], [], [Use partially unsorted event set])],
          [])

]) dnl end CHECK_PARTIALLY_UNSORTED_EVENT_SET

