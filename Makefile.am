#SUBDIRS = src
sbin_PROGRAMS = \
	reactord

reactord_CFLAGS= $(GLIB_CFLAGS)

reactord_SOURCES = \
	src/main.c \
	src/action.h \
	src/database.c \
	src/eventnotice.c \
	src/eventnotice.h \
	src/macros.h \
	src/state.c \
	src/state.h \
	src/transition.c \
	src/transition.h \
	src/user.c \
	src/user.h

reactor_LIBS= $(GLIB_LIBS)
