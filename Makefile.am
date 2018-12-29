# Makefile.am Print Directory (pd)

bin_PROGRAMS = pd

pd_SOURCES = pd.c

if HAVE_SOLARIS
pd_CPPFLAGS = -D_SOLARIS_ACL -D_DATE_YMD `getconf LFS_CFLAGS`
pd_LDFLAGS = -lcurses
endif

if HAVE_LINUX
pd_CPPFLAGS = -D_DATE_YMD `getconf LFS_CFLAGS`
pd_LDFLAGS = -lncurses
endif

man_MANS = pd.1
