#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([Print Directory (pd)], [5.1.1], [Author sfehrman@computer.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([pd.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

# Get host information
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdlib.h sys/acl.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_STRUCT_TM

# check the OS type
case "$host" in
	*-*-linux*)
		AM_CONDITIONAL(HAVE_LINUX, true)
		AM_CONDITIONAL(HAVE_SOLARIS, false)
	;;
	*-*-solaris* | *-*-sunos*)
		AM_CONDITIONAL(HAVE_SOLARIS, true)
		AM_CONDITIONAL(HAVE_LINUX, false)
	;;
esac

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([acl getcwd strerror])
AC_OUTPUT
