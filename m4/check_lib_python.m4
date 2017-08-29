dnl Check for Python

dnl Usage: CHECK_LIB_PYTHON

AC_DEFUN([CHECK_LIB_PYTHON],
[
    dnl This allows the user to override default include directory
    AC_ARG_WITH([python-includedir],
                [AS_HELP_STRING([--with-python-includedir(=INCLUDEDIR)], [location of Python.h])],
                [want_python_includedir="$withval"],
                [want_python_includedir="no"])

    AS_IF([test "x$want_python_includedir" = "xno"], [CXXFLAGS="$CXXFLAGS $(python-config --includes)"],
                [CXXFLAGS="$CXXFLAGS -isystem$withval"])

    AC_CHECK_HEADER([Python.h],
                [AC_MSG_RESULT([Successfully found Python.h.])],
                [AC_MSG_ERROR([Could not find Python.h. Install Python or use --with-python-includedir to specify include path])])

    dnl This allows user to override default library directory
    AC_ARG_WITH([python-libdir],
                [AS_HELP_STRING([--with-python-libdir(=LIBDIR)],[location of Python library])],
                [want_python_libdir="$withval"],
                [want_python_libdir="no"])

    AS_IF([test "x$want_python_libdir" = "xno"], [LDFLAGS="$LDFLAGS $(python-config --libs)"],
                [LDFLAGS="$LDFLAGS -L$withval"])

    AC_SEARCH_LIBS([Py_Initialize],
                [python],
                [],
                [AC_MSG_ERROR([Could not find lpython Use --with-python-libdir to specify library path])])

]) dnl end CHECK_LIB_PYTHON

