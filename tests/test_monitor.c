#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <libxklavier/xklavier.h>
#include <libxklavier/xklavier_config.h>

extern void XklConfigDump( FILE* file,
                           XklConfigRecPtr data );

static void printUsage()
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
  int debugLevel = -1;
  XkbEvent ev;
  Display* dpy;
  int listenerType = 0, lt;
  int listenerTypes[] = { XKLL_MANAGE_LAYOUTS, 
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
        printUsage();
        exit(0);
      case 'd':
        debugLevel = atoi( optarg );
        break;
      case 'l':
        lt = optarg[0] - '1';
        if( lt >= 0 && lt < sizeof(listenerTypes)/sizeof(listenerTypes[0]) )
          listenerType |= listenerTypes[lt];
        break;
      default:
        fprintf( stderr, "?? getopt returned character code 0%o ??\n", c );
        printUsage();
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
  if( !XklInit( dpy ) )
  {
    XklConfigRec currentConfig;
    if( debugLevel != -1 )
      XklSetDebugLevel( debugLevel );
    XklDebug( 0, "Xklavier initialized\n" );
    XklConfigInit();
    XklConfigLoadRegistry();
    XklDebug( 0, "Xklavier registry loaded\n" );

    XklConfigRecInit( &currentConfig );
    XklConfigGetFromServer( &currentConfig );

    XklDebug( 0, "Now, listening...\n" );
    XklStartListen( listenerType );

    while (1) 
    {
      XNextEvent( dpy, &ev.core );
      if ( XklFilterEvents( &ev.core ) )
        XklDebug( 200, "Unknown event %d\n", ev.type );
    }

    XklStopListen();

    XklConfigRecDestroy( &currentConfig );

    XklConfigFreeRegistry();
    XklConfigTerm();
    XklDebug( 0, "Xklavier registry freed\n" );
    XklDebug( 0, "Xklavier terminating\n" );
    XklTerm();
  } else
  {
    fprintf( stderr, "Could not init Xklavier: %s\n", XklGetLastError() );
    exit(2);
  }
  printf( "closing display: %p\n", dpy );
  XCloseDisplay(dpy);
  return 0;
}
