sbin_PROGRAMS = reactord

AM_CFLAGS = -std=c99 $(GLIB_CFLAGS) -I$(top_srcdir)/lib/ -g -O0
AM_LDFLAGS = $(GLIB_LIBS)       

reactord_SOURCES =	src/reactord.h \
				src/reactord.c \
				src/user.c \
				lib/reactor.h \
				lib/log.c \
				lib/eventnotice.c \
				lib/state.c \
				lib/transition.c  
