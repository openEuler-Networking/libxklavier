#include <stdio.h>
#include <X11/Xlib.h>
#include <libxklavier/xklavier.h>
#include <libxklavier/xklavier_config.h>

static void dump( XklConfigRecPtr ptr )
{
  int i,j;
  char**p;
  XklDebug( 0, "  model: [%s]\n", ptr->model );

  XklDebug( 0, "  layouts(%d):\n", ptr->numLayouts );
  p = ptr->layouts;
  for( i = ptr->numLayouts, j = 0; --i >= 0; )
    XklDebug( 0, "  %d: [%s]\n", j++, *p++ );

  XklDebug( 0, "  variants(%d):\n", ptr->numVariants );
  p = ptr->variants;
  for( i = ptr->numVariants, j = 0; --i >= 0; )
    XklDebug( 0, "  %d: [%s]\n", j++, *p++ );

  XklDebug( 0, "  options(%d):\n", ptr->numOptions );
  p = ptr->options;
  for( i = ptr->numOptions, j = 0; --i >= 0; )
    XklDebug( 0, "  %d: [%s]\n", j++, *p++ );
}

int main( int argc, const char * argv[] )
{
  Display* dpy = XOpenDisplay( NULL );
  if ( dpy == NULL )
  {
    fprintf( stderr, "Could not open display\n" );
    exit(1);
  }
  printf( "opened display: %p\n", dpy );
  if ( !XklInit( dpy ) )
  {
    XklConfigRec r1, r2;
    XklDebug( 0, "Xklavier initialized\n" );
    XklConfigInit();
    XklConfigLoadRegistry();
    XklDebug( 0, "Xklavier registry loaded\n" );
    XklDebug( 0, "Multiple layouts are %ssupported\n",
      XklMultipleLayoutsSupported() ? "" : "not " );

    XklConfigRecInit( &r1 );
    XklConfigRecInit( &r2 );

    if ( XklConfigGetFromServer( &r1 ) )
    {
      XklDebug( 0, "Got config from the server\n" );
      dump( &r1 );
    }
    if ( XklConfigGetFromBackup( &r2 ) )
    {
      XklDebug( 0, "Got config from the backup\n" );
      dump( &r2 );
    }

    XklConfigRecDestroy( &r2 );
    XklConfigRecDestroy( &r1 );

    XklConfigFreeRegistry();
    XklConfigTerm();
    XklDebug( 0, "Xklavier registry freed\n" );
    XklDebug( 0, "Xklavier terminating\n" );
    XklTerm();
  } else
  {
    fprintf( stderr, "Could not init Xklavier\n" );
    exit(2);
  }
  printf( "closing display: %p\n", dpy );
  XCloseDisplay(dpy);
  return 0;
}
