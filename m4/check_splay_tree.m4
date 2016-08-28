dnl Check for debug build

dnl Usage: CHECK_SPLAY_TREE

AC_DEFUN([CHECK_SPLAY_TREE],
[
    AC_ARG_WITH([splay-tree],
                [AS_HELP_STRING([--with-splay-tree], [Use splay tree as scheduler])],
                [want_splaytree="$withval"],
                [want_splaytree="no"])

    AS_IF([test "x$want_splaytree" = "xyes"],
          [AC_DEFINE([SPLAY_TREE], [], [Use splay tree as scheduler])],
          [])

]) dnl end CHECK_SPLAY_TREE

