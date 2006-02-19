#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <libxklavier/xklavier.h>
#include <libxklavier/xklavier_config.h>

extern void xkl_config_dump( FILE* file,
                             XklConfigRec *data );

static void print_usage()
{
  printf( "Usage: test_monitor (-l1)(-l2)(-l3)(-h)(-d <debugLevel>)\n" );
  printf( "Options:\n" );
  printf( "         -d - Set the debug level (by default, 0)\n" ); 
  printf( "         -h - Show this help\n" );
  printf( "         -l1 - listen to manage layouts\n" );
  printf( "         -l2 - listen to manage window states\n" );
  printf( "         -l3 - listen to track the keyboard state\n" );
}

int main( int argc, char * argv[] )
{
  int c;
  int debug_level = -1;
  XkbEvent ev;
  Display* dpy;
  int listener_type = 0, lt;
  int listener_types[] = { XKLL_MANAGE_LAYOUTS, 
                           XKLL_MANAGE_WINDOW_STATES,
                           XKLL_TRACK_KEYBOARD_STATE };

  while (1)
  {
    c = getopt( argc, argv, "hd:l:" );
    if ( c == -1 )
      break;
    switch (c)
    {
      case 'h':
        print_usage();
        exit(0);
      case 'd':
        debug_level = atoi( optarg );
        break;
      case 'l':
        lt = optarg[0] - '1';
        if( lt >= 0 && lt < sizeof(listener_types)/sizeof(listener_types[0]) )
          listener_type |= listener_types[lt];
        break;
      default:
        fprintf( stderr, "?? getopt returned character code 0%o ??\n", c );
        print_usage();
        exit(0);
    }
  }

  dpy = XOpenDisplay( NULL );
  if ( dpy == NULL )
  {
    fprintf( stderr, "Could not open display\n" );
    exit(1);
  }
  printf( "opened display: %p\n", dpy );
  if( !xkl_init( dpy ) )
  {
    XklConfigRec current_config;
    if( debug_level != -1 )
      xkl_set_debug_level( debug_level );
    xkl_debug( 0, "Xklavier initialized\n" );
    xkl_config_init();
    xkl_config_registry_load();
    xkl_debug( 0, "Xklavier registry loaded\n" );

    xkl_config_rec_init( &current_config );
    xkl_config_get_from_server( &current_config );

    xkl_debug( 0, "Now, listening...\n" );
    xkl_listen_start( listener_type );

    while (1) 
    {
      XNextEvent( dpy, &ev.core );
      if ( xkl_events_filter( &ev.core ) )
        xkl_debug( 200, "Unknown event %d\n", ev.type );
    }

    xkl_listen_stop();

    xkl_config_rec_destroy( &current_config );

    xkl_config_registry_free();
    xkl_config_term();
    xkl_debug( 0, "Xklavier registry freed\n" );
    xkl_debug( 0, "Xklavier terminating\n" );
    xkl_term();
  } else
  {
    fprintf( stderr, "Could not init Xklavier: %s\n", xkl_get_last_error() );
    exit(2);
  }
  printf( "closing display: %p\n", dpy );
  XCloseDisplay(dpy);
  return 0;
}
