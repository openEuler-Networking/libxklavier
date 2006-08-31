if LIBXKBFILE_PRESENT
  LIBXKBFILE_PRESENT_CFLAG = -DLIBXKBFILE_PRESENT=1
  LIBXKBFILE_PRESENT_LDFLAGS = -lxkbfile
endif

if ENABLE_XKB_SUPPORT
  ENABLE_XKB_SUPPORT_CFLAG = -DENABLE_XKB_SUPPORT=1
else
  ENABLE_XKB_SUPPORT_CFLAG = -DDISABLE_XKB_SUPPORT=1
endif

if ENABLE_XMODMAP_SUPPORT
  ENABLE_XMODMAP_SUPPORT_CFLAG = -DENABLE_XMODMAP_SUPPORT=1
else
  ENABLE_XMODMAP_SUPPORT_CFLAG = -DDISABLE_XMODMAP_SUPPORT=1
endif

EXTRA_DIST=marshal.list

GLIB_GENMARSHAL = $(shell pkg-config --variable=glib_genmarshal glib-2.0)

xkl_engine_marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=xkl_engine marshal.list --header > xkl_engine_marshal.h

xkl_engine_marshal.c: xkl_engine_marshal.h
	$(GLIB_GENMARSHAL) --prefix=xkl_engine marshal.list --body > xkl_engine_marshal.c

AM_CFLAGS=-Wall -DDATA_DIR=\"$(datadir)/$(PACKAGE)\" \
  -I. -I$(includedir) $(X_CFLAGS) -I$(top_srcdir) \
  $(XML_CFLAGS) $(GLIB_CFLAGS) \
  $(LIBXKBFILE_PRESENT_CFLAG) \
  $(ENABLE_XKB_SUPPORT_CFLAG) \
  $(ENABLE_XMODMAP_SUPPORT_CFLAG)

lib_LTLIBRARIES = libxklavier.la
noinst_HEADERS = xklavier_private.h xklavier_private_xkb.h xklavier_private_xmm.h

xklavierincdir = $(includedir)/libxklavier
xklavierinc_HEADERS = xklavier.h xkl_config_registry.h xkl_engine.h \
                      xkl_config_rec.h xkl_config_item.h xkl_engine_marshal.h

libxklavier_la_SOURCES = xklavier.c xklavier_evt.c xklavier_config.c \
	xklavier_xkb.c xklavier_evt_xkb.c xklavier_config_xkb.c xklavier_toplevel.c \
	xklavier_xmm.c xklavier_xmm_opts.c xklavier_evt_xmm.c xklavier_config_xmm.c \
	xklavier_util.c xklavier_config_i18n.c xklavier_props.c xklavier_dump.c xkl_engine_marshal.c \
	$(noinst_HEADERS) $(xklavierinc_HEADERS)
libxklavier_la_LDFLAGS = -version-info @VERSION_INFO@ \
 $(XML_LIBS) $(GLIB_LIBS) \
 $(LIBXKBFILE_PRESENT_LDFLAGS) \
 $(X_LIBS) $(LIBICONV) 


