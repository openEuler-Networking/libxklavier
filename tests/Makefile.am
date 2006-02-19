noinst_PROGRAMS=test_config test_monitor

test_config_SOURCES=test_config.c

test_monitor_SOURCES=test_monitor.c

AM_CFLAGS=-Wall -Werror -I$(includedir) -I$(x_includes) -I$(top_srcdir) $(GLIB_CFLAGS)

AM_LDFLAGS=$(top_builddir)/libxklavier/libxklavier.la -L$(x_libraries) -lX11 $(GLIB_LIBS)

