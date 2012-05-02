AM_CFLAGS =	$(GLIB_CFLAGS) \
		$(LIBEVENT_CFLAGS) \
		-I$(top_srcdir)/lib/ \
		-fvisibility=hidden \
		-pthread

AM_LDFLAGS =	$(GLIB_LIBS) \
		$(LIBEVENT_LIBS) \
		-pthread
##############
# libreactor #
##############
LIBREACTOR_CURRENT = 0
LIBREACTOR_REVISION = 0
LIBREACTOR_AGE = 0

include_HEADERS = lib/libreactor.h
lib_LTLIBRARIES = lib/libreactor.la

lib_libreactor_la_SOURCES =	lib/libreactor-private.h \
				lib/log-private.c \
				lib/cntrl.c \
				lib/util-private.c
 
lib_libreactor_la_LDFLAGS =	$(AM_LDFLAGS)
				-version-info $(LIBREACTOR_CURRENT):$(LIBREACTOR_REVISION):$(LIBREACTOR_AGE)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib/libreactor.pc

#########################
# reactord & reactorctl #
#########################
sbin_PROGRAMS = src/reactord
bin_PROGRAMS = src/reactorctl				

src_reactord_modules = 	src/reactor.h \
			src/user.c \
			src/rules.c \
			src/eventnotice.c \
			src/state.c \
			src/transition.c \
			src/action.c \
			src/remote.c

src_reactord_SOURCES =	src/reactord.c \
			$(src_reactord_modules)

src_reactord_LDADD =	lib/libreactor.la

src_reactorctl_SOURCES =	src/reactor.h \
				src/reactorctl.c

src_reactorctl_LDADD =	lib/libreactor.la

#########
# tests #
#########
TESTS = 		tests/tests
check_PROGRAMS =	$(TESTS)

tests_tests_SOURCES = 	tests/tests.c \
			tests/tests.h \
			tests/cntrl-tests.c \
			tests/rules-tests.c

tests_tests_CFLAGS = 	@CHECK_CFLAGS@ \
			-I$(top_srcdir)/src/ \
			$(AM_CFLAGS)

tests_tests_LDADD = 	lib/libreactor.la \
			@CHECK_LIBS@

# Hooks
install-exec-hook:
	groupadd events || true
