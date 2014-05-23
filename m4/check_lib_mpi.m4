dnl Check for cppunit

dnl Usage: CHECK_LIB_MPI

AC_DEFUN([CHECK_LIB_MPI],
[
AC_CACHE_CHECK([for location of libmpi], [cl_cv_lib_mpi],
  [AC_ARG_WITH(mpi,
    AS_HELP_STRING([--with-mpi],
      [location of mpich (default is /usr/lib)]),
      [
        case "${withval}" in
          "")   cl_cv_lib_mpi=/usr/lib ;;
          *)    cl_cv_lib_mpi=$withval ;;
        esac
      ],
      [cl_cv_lib_mpi=/usr]
  )]
)

if test $cl_cv_lib_mpi != /usr/lib ; then
LDFLAGS="$LDFLAGS -L$cl_cv_lib_mpi"
fi

AC_CACHE_CHECK([for location of mpi.h], [cl_cv_mpi_h],
  [AC_ARG_WITH(mpiheader,
    AS_HELP_STRING([--with-mpiheader],
      [location of mpi.h (default is /usr/include)]),
      [
        case "${withval}" in
          "")   cl_cv_mpi_h=/usr/include ;;
          *)    cl_cv_mpi_h=$withval ;;
        esac
      ],
      [cl_cv_mpi_h=/usr/include]
  )]
)

if test $cl_cv_mpi_h != /usr/include ; then
CPPFLAGS="$CPPFLAGS -isystem$cl_cv_mpi_h"
fi

havempi=no
havempich=no

AC_CHECK_HEADER( mpi.h,,
  AC_MSG_ERROR(Couldn't find mpi.h. You probably need to install MPI or define an include path with the --with-mpiheader option.) )
AC_CHECK_LIB( mpi, MPI_Init, havempi=yes,
  AC_CHECK_LIB( mpich, MPI_Init, havempich=yes,
    AC_MSG_ERROR([Could neither find libmpi.a nor libmpich.a. You probably need to define the lib path with the --with-mpi option]), -lmpl))

AM_CONDITIONAL(HAVE_MPICH, test $havempich = yes)
AM_CONDITIONAL(HAVE_MPI, test $havempi = yes)

]) dnl end CHECK_LIB_CPPUNIT
