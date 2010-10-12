noinst_PROGRAMS = mtdev-mapgen mtdev-matching
bin_PROGRAMS = mtdev-test

INCLUDES=-I$(top_srcdir)/include/

mtdev_mapgen_SOURCES = mtdev-mapgen.c

mtdev_matching_SOURCES = mtdev-matching.c
mtdev_matching_LDFLAGS = -L$(top_builddir)/src/.libs/ -lmtdev

mtdev_test_SOURCES = mtdev-test.c
mtdev_test_LDFLAGS = -lmtdev -L$(top_builddir)/src/.libs/
