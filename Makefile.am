SUBDIRS = libxklavier doc

EXTRA_DIST = libxklavier.spec libxklavier.spec.in \
             Doxyfile Doxyfile.in autogen.sh \
             CREDITS libxklavier.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxklavier.pc

doxydocs: libxklavier/xklavier.h libxklavier/xklavier_config.h
	if test "x$(DO_DOXYGEN)" = "xyes" ; then \
	  doxygen Doxyfile; \
	fi;

all: doxydocs

.PHONY: doxydocs
