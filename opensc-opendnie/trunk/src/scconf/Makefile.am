include $(top_srcdir)/win32/ltrc.inc

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in 
DISTCLEANFILES = lex-parse.c
EXTRA_DIST = Makefile.mak

dist_noinst_DATA = README.scconf lex-parse.l
noinst_HEADERS = internal.h scconf.h
noinst_PROGRAMS = test-conf
noinst_LTLIBRARIES = libscconf.la

INCLUDES = -I$(top_srcdir)/src

libscconf_la_SOURCES = scconf.c parse.c write.c sclex.c 

test_conf_SOURCES = test-conf.c
test_conf_LDADD = libscconf.la $(top_builddir)/src/common/libcompat.la
