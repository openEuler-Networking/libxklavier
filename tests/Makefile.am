noinst_PROGRAMS=test_config test_monitor

test_config_SOURCES=test_config.c

test_monitor_SOURCES=test_monitor.c

AM_CFLAGS=-Wall -Werror -I$(includedir) $(X_CFLAGS) -I$(top_srcdir) $(GLIB_CFLAGS)

AM_LDFLAGS=$(top_builddir)/libxklavier/libxklavier.la $(X_LIBS) -lX11 $(GLIB_LIBS)

