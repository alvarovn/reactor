sbin_PROGRAMS = reactord
bin_PROGRAMS = reactorctl

AM_CFLAGS = -std=c99 $(GLIB_CFLAGS) $(LIBEVENT_CFLAGS) -I$(top_srcdir)/lib/
AM_LDFLAGS = $(GLIB_LIBS) $(LIBEVENT_LIBS)

reactord_SOURCES =	src/reactord.h \
			src/reactord.c \
			src/user.c \
			src/cntrl.c \
			lib/reactor.h \
			lib/log.c \
			lib/eventnotice.c \
			lib/state.c \
			lib/transition.c

reactorctl_SOURCES =	src/reactorctl.c \
			src/reactord.h \
			lib/reactor.h \
			lib/log.c \
			src/cntrl.c