SUBDIRS = libxklavier doc tests

EXTRA_DIST = libxklavier.spec libxklavier.spec.in \
             autogen.sh \
             CREDITS libxklavier.pc.in gtk-doc.make \
             $(config_xml_DATA)

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxklavier.pc
