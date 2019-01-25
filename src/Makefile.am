ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libfflag.la
libfflag_la_SOURCES = libfflag.c
include_HEADERS = libfflag.h


bin_PROGRAMS = waitflag
waitflag_SOURCES=waitflag.c
waitflag_LDADD = libfflag.la
