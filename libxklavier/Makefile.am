if XKB_HEADERS_PRESENT
  XKB_HEADERS_PRESENT_CFLAG = -DXKB_HEADERS_PRESENT=1
endif

if ENABLE_XKB_SUPPORT
  ENABLE_XKB_SUPPORT_CFLAG = -DENABLE_XKB_SUPPORT=1
else
  ENABLE_XKB_SUPPORT_CFLAG = -DDISABLE_XKB_SUPPORT=1
endif

if ENABLE_XMM_SUPPORT
  ENABLE_XMM_SUPPORT_CFLAG = -DENABLE_XMM_SUPPORT=1
else
  ENABLE_XMM_SUPPORT_CFLAG = -DDISABLE_XMM_SUPPORT=1
endif

AM_CFLAGS=-Wall -Werror -DDATA_DIR=\"$(datadir)/$(PACKAGE)\" \
  -I. -I$(includedir) -I$(x_includes) -I$(top_srcdir) \
  $(XML_CFLAGS) $(GLIB_CFLAGS) \
  $(XKB_HEADERS_PRESENT_CFLAG) \
  $(ENABLE_XKB_SUPPORT_CFLAG) \
  $(ENABLE_XMM_SUPPORT_CFLAG)

lib_LTLIBRARIES = libxklavier.la
noinst_HEADERS = xklavier_private.h xklavier_private_xkb.h xklavier_private_xmm.h

xklavierincdir = $(includedir)/libxklavier
xklavierinc_HEADERS = xklavier.h xklavier_config.h

libxklavier_la_SOURCES = xklavier.c xklavier_evt.c xklavier_config.c \
	xklavier_xkb.c xklavier_evt_xkb.c xklavier_config_xkb.c xklavier_toplevel.c \
	xklavier_xmm.c xklavier_xmm_opts.c xklavier_evt_xmm.c xklavier_config_xmm.c \
	xklavier_util.c xklavier_config_i18n.c xklavier_props.c xklavier_dump.c \
	$(noinst_HEADERS) $(xklavierinc_HEADERS)
libxklavier_la_LDFLAGS = -version-info @VERSION_INFO@ \
 $(XML_LIBS) $(GLIB_LIBS) \
 -lxkbfile -L$(x_libraries) $(LIBICONV) 


