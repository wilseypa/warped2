dnl Check for debug build

dnl Usage: CHECK_SPLAY_TREE_SCHEDULER

AC_DEFUN([CHECK_SPLAY_TREE_SCHEDULER],
[
    AC_ARG_WITH([splay-tree-scheduler],
                [AS_HELP_STRING([--with-splay-tree-scheduler], [Use splay tree as scheduler])],
                [want_splaytree="$withval"],
                [want_splaytree="no"])

    AS_IF([test "x$want_splaytree" = "xyes"],
          [AC_DEFINE([SPLAY_TREE_SCHEDULER], [], [Use splay tree as scheduler])],
          [])

]) dnl end CHECK_SPLAY_TREE_SCHEDULER

