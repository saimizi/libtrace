ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libfflag.la
libfflag_la_SOURCES = libfflag.c
include_HEADERS = libfflag.h


bin_PROGRAMS = waitfflag
waitfflag_SOURCES=waitfflag.c
waitfflag_LDADD = libfflag.la

install-exec-hook:
	unlink $(DESTDIR)$(bindir)/waitfflagon 2>/dev/null || true
	unlink $(DESTDIR)$(bindir)/waitfflagoff 2>/dev/null || true
	unlink $(DESTDIR)$(bindir)/setfflag 2>/dev/null || true
	unlink $(DESTDIR)$(bindir)/clearfflag 2>/dev/null || true
	cd $(DESTDIR)$(bindir); ln -s waitfflag waitfflagon
	cd $(DESTDIR)$(bindir); ln -s waitfflag waitfflagoff
	cd $(DESTDIR)$(bindir); ln -s waitfflag setfflag
	cd $(DESTDIR)$(bindir); ln -s waitfflag clearfflag
