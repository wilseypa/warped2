dnl Check for debug build

dnl Usage: CHECK_SCHEDULE_QUEUE_LOCK

AC_DEFUN([CHECK_SCHEDULE_QUEUE_LOCK],
[
    AC_ARG_WITH([schedule-queue-spinlocks],
                [AS_HELP_STRING([--with-schedule-queue-spinlocks], [Use spinlocks for schedule queues])],
                [want_spinlocks="$withval"],
                [want_spinlocks="no"])

    AS_IF([test "x$want_spinlocks" = "xyes"],
          [AC_DEFINE([SCHEDULE_QUEUE_SPINLOCKS], [], [Use spinlocks for schedule queues])],
          [])

]) dnl end CHECK_SCHEDULE_QUEUE_LOCK

