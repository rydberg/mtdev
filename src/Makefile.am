lib_LTLIBRARIES = libmtdev.la

libmtdev_la_LDFLAGS = -version-info @LIB_VERSION@

libmtdev_la_SOURCES = \
	caps.c \
	core.c \
	iobuf.c \
	match.c

AM_CFLAGS = $(CWARNFLAGS)

INCLUDES = -I$(top_srcdir)/include/

libmtdevincludedir = $(includedir)
libmtdevinclude_HEADERS = $(top_srcdir)/include/mtdev.h
