#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([drpc], [0.1.0], [https://github.com/CiYong/drpc])
AC_CONFIG_SRCDIR([src/drpc.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

AC_CONFIG_MACRO_DIRS([m4])
m4_pattern_allow([AC_PROG_CC_C99])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CXX

# Libtool configuration for different targets
AM_PROG_AR
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([socket])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
