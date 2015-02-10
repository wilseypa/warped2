dnl Check for mpi

dnl Usage: CHECK_LIB_MPI

AC_DEFUN([CHECK_LIB_MPI],
[
    DEFAULT_LIBDIR="/usr/lib"
    DEFAULT_INCLUDEDIR="/usr/include"

    DEFAULT_LIBDIR_MPICH="/usr/lib/mpich"
    DEFAULT_INCLUDEDIR_MPICH="/usr/include/mpich"

    DEFAULT_LIBDIR_MPICH2="/usr/lib/mpich2"
    DEFAULT_INCLUDEDIR_MPICH2="/usr/include/mpich2"

    DEFAULT_LIBDIR_OPENMPI="/usr/lib/openmpi"
    DEFAULT_INCLUDEDIR_OPENMPI="/usr/include/openmpi"

    AC_ARG_WITH([mpi],
                [AS_HELP_STRING([--with-mpi(=IMPLEMENTATION)], [mpi implementation. Valid options include MPICH and OPENMPI])],
                [MPI_IMPLEMENTATION="$withval"],
                [MPI_IMPLEMENTATION=""])

    MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR"
    dnl This allows the user to override default include directory
    AC_ARG_WITH([mpi-includedir],
                [AS_HELP_STRING([--with-mpi-includedir(=INCLUDEDIR)], [location of mpi.h])],
                [MPI_INCLUDEDIR="$withval"; default_includedir=no],
                [default_includedir=yes])

    MPI_LIBDIR="$DEFAULT_LIBDIR"
    dnl This allows user to override default library directory
    AC_ARG_WITH([mpi-libdir],
                [AS_HELP_STRING([--with-mpi-libdir(=LIBDIR)],[location of mpi library])],
                [MPI_LIBDIR="$withval"; default_libdir=no],
                [default_libdir=yes])

    dnl Check include paths
    AS_IF([test "$default_includedir" = "yes"],
          [AS_IF([test -e $DEFAULT_INCLUDEDIR_MPICH2/mpi.h && test "$MPI_IMPLEMENTATION" != "OPENMPI"],
                 [MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_MPICH2"; MPI_IMPLEMENTATION="MPICH"],
          [AS_IF([test -e $DEFAULT_INCLUDEDIR_MPICH/mpi.h && test "$MPI_IMPLEMENTATION" != "OPENMPI"],
                 [MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_MPICH"; MPI_IMPLEMENTATION="MPICH"],
          [AS_IF([test -e $DEFAULT_INCLUDEDIR_OPENMPI/mpi.h],
                 [MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_OPENMPI"; MPI_IMPLEMENTATION="OPENMPI"]
    )])])])

    AC_MSG_RESULT([MPI include path: $MPI_INCLUDEDIR])
    CPPFLAGS="$CPPFLAGS -isystem$MPI_INCLUDEDIR"

    dnl Check library paths
    AS_IF([test "$default_libdir" = "yes"],
          [AS_IF([test -e $DEFAULT_LIBDIR_MPICH2/lib/mpich.so],
                 [MPI_LIBDIR="$DEFAULT_LIBDIR_MPICH2"],
          [AS_IF([test -e $DEFAULT_LIBDIR_MPICH/lib/mpich.so],
                 [MPI_LIBDIR="$DEFAULT_LIBDIR_MPICH"],
          [AS_IF([test -e $DEFAULT_LIBDIR_OPENMPI/lib/mpi.so && "$MPI_IMPLEMENTATION" != "MPICH"],
                 [MPI_LIBDIR="$DEFAULT_LIBDIR_OPENMPI"]
    )])])])

    AC_MSG_RESULT([MPI library path: $MPI_LIBDIR])
    LDFLAGS="$LDFLAGS -L$MPI_LIBDIR"

    AC_CHECK_HEADER([mpi.h],
                    [AC_MSG_RESULT([Successfully found mpi.h. Using $MPI_IMPLEMENTATION])],
                    [AC_MSG_ERROR([Could not find mpi.h. Install mpi or use --with-mpi-includedir to specify include path])])

    AS_IF([test "$MPI_IMPLEMENTATION" = "MPICH"],
          [AC_CHECK_LIB([mpich],
                        [MPI_Init],
                        [],
                        [AC_MSG_ERROR([Could not find libmpich.so. Use --with-mpi-libdir to specify library path])],
                        [-lmpl])
          AC_CHECK_LIB([mpl],
                       [MPL_trinit],
                       [],
                       [AC_MSG_ERROR([Could not find libmpl.so])],
                       [-lmpl])],
    [AS_IF([test "$MPI_IMPLEMENTATION" = "OPENMPI"],
           [AC_CHECK_LIB([mpi],
                         [MPI_Init],
                         [],
                         [AC_MSG_ERROR([Could not find libmpi.so. Use --with-mpi-libdir to specify library path])],
                         [])],
    [AC_MSG_ERROR([Invalid MPI configuration. Use ./configure --help for more details])])])

    AM_CONDITIONAL([HAVE_MPICH], [test "$MPI_IMPLEMENTATION" = "MPICH"])
    AM_CONDITIONAL([HAVE_MPI], [test "$MPI_IMPLEMENTATION" = "OPENMPI"])

]) dnl end CHECK_MPI

