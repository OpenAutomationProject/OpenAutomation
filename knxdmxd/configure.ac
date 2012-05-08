#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT(knxdmxd)
AM_INIT_AUTOMAKE(knxdmxd, 0.01)

AC_CONFIG_SRCDIR([src])
#AC_CONFIG_HEADERS([config.h:config.h.in])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB([eibclient], [EIBClose], ,[AC_MSG_ERROR(
[Couldn't find libeibclient...try installing the eibclient-dev package] )])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([json], [json_object_get], ,[AC_MSG_ERROR(
[Couldn't find libjson0...try installing the libjson0-dev package] )])

dnl ************************************
dnl Check for libola
dnl ************************************
PKG_CHECK_MODULES([OLA], [libola], ,[AC_MSG_ERROR(
[Couldn't find libola...try installing the ola package] )])
AC_SUBST(OLA_LIBS)


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK

AC_OUTPUT(src/Makefile Makefile)

