ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libxklavier doc tests

EXTRA_DIST = libxklavier.spec libxklavier.spec.in \
             autogen.sh \
             CREDITS libxklavier.pc.in \
             $(config_xml_DATA)

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxklavier.pc
