AM_CFLAGS = 	-std=c99 \
		$(GLIB_CFLAGS) \
		$(LIBEVENT_CFLAGS) \
		-I$(top_srcdir)/lib/

AM_LDFLAGS =	$(GLIB_LIBS) \
		$(LIBEVENT_LIBS)

# libreactor
LIBREACTOR_CURRENT = 0
LIBREACTOR_REVISION = 0
LIBREACTOR_AGE = 0

include_HEADERS = lib/libreactor.h
lib_LTLIBRARIES = lib/libreactor.la

lib_libreactor_la_SOURCES =	lib/libreactor-private.h \
				lib/log-private.c \
				lib/cntrl-private.c \
				lib/cntrl.c
 
lib_libreactor_la_LDFLAGS =	$(AM_LDFLAGS)
				-version-info $(LIBREACTOR_CURRENT):$(LIBREACTOR_REVISION):$(LIBREACTOR_AGE)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib/libreactor.pc

# reactord & reactorctl
sbin_PROGRAMS = src/reactord
bin_PROGRAMS = src/reactorctl

src_common_sources =	src/reactor.h
				

src_reactord_SOURCES =	$(src_common_sources) \
			src/reactord.c \
			src/user.c \
			src/rules.c \
			src/eventnotice.c \
			src/state.c \
			src/transition.c \
			src/action.c

src_reactord_LDADD =	lib/libreactor.la

src_reactorctl_SOURCES =	$(reactor_common_sources) \
				src/reactorctl.c

src_reactorctl_LDADD =	lib/libreactor.la
