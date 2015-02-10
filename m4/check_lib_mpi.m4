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
    if test "$default_includedir" = "yes" ; then
        if test -e $DEFAULT_INCLUDEDIR_MPICH2/mpi.h && test "$MPI_IMPLEMENTATION" != "OPENMPI" ; then
            MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_MPICH2"
            MPI_IMPLEMENTATION="MPICH"
        elif test -e $DEFAULT_INCLUDEDIR_MPICH/mpi.h && test "$MPI_IMPLEMENTATION" != "OPENMPI" ; then
            MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_MPICH"
            MPI_IMPLEMENTATION="MPICH"
        elif test -e $DEFAULT_INCLUDEDIR_OPENMPI/mpi.h ; then
            MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR_OPENMPI"
            MPI_IMPLEMENTATION="OPENMPI"
        fi
    fi

    AC_MSG_RESULT([MPI include path: $MPI_INCLUDEDIR])
    CPPFLAGS="$CPPFLAGS -isystem$MPI_INCLUDEDIR"

    dnl Check library paths
    if test "$default_libdir" = "yes" ; then
        if test -e $DEFAULT_LIBDIR_MPICH2/lib/mpich.so ; then
            MPI_LIBDIR="$DEFAULT_LIBDIR_MPICH2"
        elif test -e $DEFAULT_LIBDIR_MPICH/lib/mpich.so ; then
            MPI_LIBDIR="$DEFAULT_LIBDIR_MPICH"
        elif test -e $DEFAULT_LIBDIR_OPENMPI/lib/mpi.so && "$MPI_IMPLEMENTATION" != "MPICH" ; then
            MPI_LIBDIR="$DEFAULT_LIBDIR_OPENMPI"
        fi
    fi

    AC_MSG_RESULT([MPI library path: $MPI_LIBDIR])
    LDDFLAGS="$LDDFLAGS -L$MPI_LIBDIR"

    AC_CHECK_HEADER([mpi.h],
                    [AC_MSG_RESULT([Successfully found mpi.h. Using $MPI_IMPLEMENTATION])],
                    [AC_MSG_ERROR([Could not find mpi.h. Install mpi or use --with-mpi-includedir to specify include path])])

    if test "$MPI_IMPLEMENTATION" = "MPICH" ; then
        AC_CHECK_LIB([mpich],
                     [MPI_Init],
                     [],
                     [Could not find libmpich.so. Use --with-mpi-libdir to specify library path],
                     -lmpl)
        AC_CHECK_LIB([mpl],
                     [MPL_trinit],
                     [],
                     [Could not find libmpl.so],
                     -lmpl)
    elif test "$MPI_IMPLEMENTATION" = "OPENMPI" ; then
        AC_CHECK_LIB([mpi],
                     [MPI_Init],
                     [],
                     [Could not find libmpi.a or libmpi.so. Use --with-mpi-libdir to specify library path],
                     -lmpi_cxx)
    else
        AC_MSG_ERROR([Invalid MPI configuration. Use ./configure --help for more details])
    fi

    AM_CONDITIONAL([HAVE_MPICH], [test "$MPI_IMPLEMENTATION" = "MPICH"])
    AM_CONDITIONAL([HAVE_MPI], [test "$MPI_IMPLEMENTATION" = "OPENMPI"])

]) dnl end CHECK_MPI


