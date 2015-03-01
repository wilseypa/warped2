dnl Check for mpi

dnl Usage: CHECK_LIB_MPI

AC_DEFUN([CHECK_LIB_MPI],
[
    MPI_INCLUDEDIR="$DEFAULT_INCLUDEDIR"
    dnl This allows the user to override default include directory
    AC_ARG_WITH([mpi-includedir],
                [AS_HELP_STRING([--with-mpi-includedir(=INCLUDEDIR)], [location of mpi.h])],
                [CPPFLAGS="$CPPFLAGS -isystem$withval"],
                [])

    dnl This allows user to override default library directory
    AC_ARG_WITH([mpi-libdir],
                [AS_HELP_STRING([--with-mpi-libdir(=LIBDIR)],[location of mpi library])],
                [LDFLAG="$LDFLAGS -L$withval"],
                [])

    AC_CHECK_HEADER([mpi.h],
                    [AC_MSG_RESULT([Successfully found mpi.h.])],
                    [AC_MSG_ERROR([Could not find mpi.h. Install mpi or use --with-mpi-includedir to specify include path])])

    AC_CHECK_LIB([mpich],
                 [MPI_Init],
                 [],
                 [AC_CHECK_LIB([mpi], [MPI_Init],
                               [],
                               [AC_MSG_ERROR([Could not find libmpi.so or libmpich.so. Use --with-mpi-libdir to specify library path])],
                               [])
                 ],
                 [-lmpl])

]) dnl end CHECK_MPI

