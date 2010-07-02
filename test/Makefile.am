if BUILD_TEST

bin_PROGRAMS = mtdev-mapgen mtdev

INCLUDES=-I$(top_srcdir)/include/

mtdev_mapgen_SOURCES = mtdev-mapgen.c

mtdev_SOURCES = mtdev.c
mtdev_LDFLAGS = -lmtdev -L$(top_builddir)/src/.libs/

endif
