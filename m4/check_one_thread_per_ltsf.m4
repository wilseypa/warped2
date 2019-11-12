dnl Check for debug build

dnl Usage: CHECK_ONE_THREAD_PER_LTSF

AC_DEFUN([CHECK_ONE_THREAD_PER_LTSF],
[
     AC_ARG_WITH([one-thread-per-ltsf],
                [AS_HELP_STRING([--with-one-thread-per-ltsf], [Use one thread per schedule queue])],
                [one_tread="$withval"],
                [one_tread="no"])

    AS_IF([test "x$one_tread" = "xyes"],
          [AC_DEFINE([ONE_THREAD_PER_LTSF], [], [Use one thread per schedule queue])],
          [])

]) dnl end CHECK_ONE_THREAD_PER_LTSF

