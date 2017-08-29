dnl Check for MPI

dnl Usage: CHECK_LIB_MPI

AC_DEFUN([CHECK_LIB_MPI],
[
    dnl This allows the user to override default include directory
    AC_ARG_WITH([mpi-includedir],
                [AS_HELP_STRING([--with-mpi-includedir(=INCLUDEDIR)], [location of mpi.h])],
                [want_mpi_includedir="$withval"],
                [want_mpi_includedir="no"])

    AS_IF([test "x$want_mpi_includedir" = "xno"], [CPPFLAGS="$CPPFLAGS -I/usr/include/mpich"],
                [CXXFLAGS="$CXXFLAGS -isystem$withval"])

    AC_CHECK_HEADER([mpi.h],
                [AC_MSG_RESULT([Successfully found mpi.h.])],
                [AC_MSG_ERROR([Could not find mpi.h. Install MPI or use --with-mpi-includedir to specify include path])])

    dnl This allows user to override default library directory
    AC_ARG_WITH([mpi-libdir],
                [AS_HELP_STRING([--with-mpi-libdir(=LIBDIR)],[location of MPI library])],
                [want_mpi_libdir="$withval"],
                [want_mpi_libdir="no"])

    AS_IF([test "x$want_mpi_libdir" = "xno"], [LDFLAGS="$LDFLAGS -L/usr/lib/mpich"],
                [LDFLAGS="$LDFLAGS -L$withval"])

    AC_CHECK_LIB([mpich],
                [MPI_Init],
                [],
                [AC_CHECK_LIB([mpi], [MPI_Init],
                        [],
                        [AC_MSG_ERROR([Could not find libmpi.so or libmpich.so. Use --with-mpi-libdir to specify library path])],
                        [])
                ],
                [-lmpl])

]) dnl end CHECK_LIB_MPI

