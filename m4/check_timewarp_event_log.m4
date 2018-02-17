dnl Check for debug build

dnl Usage: CHECK_TIMEWARP_EVENT_LOG

AC_DEFUN([CHECK_TIMEWARP_EVENT_LOG],
[
    AC_ARG_WITH([timewarp-event-log],
                [AS_HELP_STRING([--with-timewarp-event-log], [Log individual event data for Time Warp])],
                [want_eventlog="$withval"],
                [want_eventlog="no"])

    AS_IF([test "x$want_eventlog" = "xyes"],
          [AC_DEFINE([TIMEWARP_EVENT_LOG], [], [Log individual event data for Time Warp])],
          [])

]) dnl end CHECK_TIMEWARP_EVENT_LOG

