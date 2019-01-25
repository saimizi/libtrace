ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libfflag.la
libfflag_la_SOURCES = libfflag.c
include_HEADERS = libfflag.h


bin_PROGRAMS = waitflag
waitflag_SOURCES=waitflag.c
waitflag_LDADD = libfflag.la

install-exec-hook:
	unlink $(DESTDIR)$(bindir)/waitflagon 2>/dev/null || true
	unlink $(DESTDIR)$(bindir)/waitflagoff 2>/dev/null || true
	ln -s $(DESTDIR)$(bindir)/waitflag $(DESTDIR)$(bindir)/waitflagon
	ln -s $(DESTDIR)$(bindir)/waitflag $(DESTDIR)$(bindir)/waitflagoff
