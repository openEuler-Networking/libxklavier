#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <libxklavier/xklavier.h>
#include <libxklavier/xklavier_config.h>

#ifdef __STRICT_ANSI__
/* these are functions which are NOT in ANSI C. 
   Probably we should provide the implementation */
extern char *strdup( const char *s );
#endif

extern void XklConfigDump( FILE* file,
                           XklConfigRecPtr data );

enum { ACTION_NONE, ACTION_GET, ACTION_SET, ACTION_WRITE };

static void printUsage()
{
  printf( "Usage: test_config (-g)|(-s -m <model> -l <layouts> -o <options>)|(-h)|(-ws)|(-wb)(-d <debugLevel>)\n" );
  printf( "Options:\n" );
  printf( "         -g - Dump the current config, load original system settings and revert back\n" ); 
  printf( "         -s - Set the configuration given my -m -l -o options. Similar to setxkbmap\n" ); 
  printf( "         -ws - Write the binary XKB config file (" PACKAGE ".xkm)\n" );
  printf( "         -wb - Write the source XKB config file (" PACKAGE ".xkb)\n" );
  printf( "         -d - Set the debug level (by default, 0)\n" );
  printf( "         -h - Show this help\n" );
}

int main( int argc, char * const argv[] )
{
  int c, i;
  int action = ACTION_NONE;
  const char* model = NULL;
  const char* layouts = NULL;
  const char* options = NULL;
  int debugLevel = -1;
  int binary = 0;
  Display *dpy;

  while (1)
  {
    c = getopt( argc, argv, "hsgm:l:o:d:w:" );
    if ( c == -1 )
      break;
    switch (c)
    {
      case 's':
        printf( "Set the config\n" );
        action = ACTION_SET;
        break;
      case 'g':
        printf( "Get the config\n" );
        action = ACTION_GET;
        break;
      case 'm':
        printf( "Model: [%s]\n", model = optarg );
        break;
      case 'l':
        printf( "Layouts: [%s]\n", layouts = optarg );
        break;
      case 'o':
        printf( "Options: [%s]\n", options = optarg );
        break;
      case 'h':
        printUsage();
        exit(0);
      case 'd':
        debugLevel = atoi( optarg );
        break;
      case 'w':
        action = ACTION_WRITE;
        binary = ( 'b' == optarg[0] );
      default:
        fprintf( stderr, "?? getopt returned character code 0%o ??\n", c );
        printUsage();
    }
  }

  if ( action == ACTION_NONE )
  {
    printUsage();
    exit( 0 );
  }

  dpy = XOpenDisplay( NULL );
  if ( dpy == NULL )
  {
    fprintf( stderr, "Could not open display\n" );
    exit(1);
  }
  printf( "opened display: %p\n", dpy );
  if ( !XklInit( dpy ) )
  {
    XklConfigRec currentConfig, r2;
    if( debugLevel != -1 )
      XklSetDebugLevel( debugLevel );
    XklDebug( 0, "Xklavier initialized\n" );
    XklConfigInit();
    XklConfigLoadRegistry();
    XklDebug( 0, "Xklavier registry loaded\n" );
    XklDebug( 0, "Bakend: [%s]\n", XklGetBackendName() );
    XklDebug( 0, "Supported features: 0x0%X\n", XklGetBackendFeatures() );
    XklDebug( 0, "Max number of groups: %d\n", XklGetMaxNumGroups() );

    XklConfigRecInit( &currentConfig );
    XklConfigGetFromServer( &currentConfig );

    switch ( action )
    {
      case ACTION_GET:
        XklDebug( 0, "Got config from the server\n" );
        XklConfigDump( stdout, &currentConfig );

        XklConfigRecInit( &r2 );

        if ( XklConfigGetFromBackup( &r2 ) )
        {
          XklDebug( 0, "Got config from the backup\n" );
          XklConfigDump( stdout, &r2 );
        }

        if ( XklConfigActivate( &r2 ) )
        {
          XklDebug( 0, "The backup configuration restored\n" );
          if ( XklConfigActivate( &currentConfig ) )
          {
            XklDebug( 0, "Reverting the configuration change\n" );
          } else
          {
            XklDebug( 0, "The configuration could not be reverted: %s\n", XklGetLastError() );
          }
        } else
        {
          XklDebug( 0, "The backup configuration could not be restored: %s\n", XklGetLastError() );
        }

        XklConfigRecDestroy( &r2 );
        break;
      case ACTION_SET:
        if ( model != NULL )
        {
          if ( currentConfig.model != NULL ) free ( currentConfig.model );
          currentConfig.model = strdup( model );
        }

        if ( layouts != NULL )
        {
          if ( currentConfig.layouts != NULL ) 
          {
            for ( i = currentConfig.numLayouts; --i >=0; )
              free ( currentConfig.layouts[i] );
            free ( currentConfig.layouts );
            for ( i = currentConfig.numVariants; --i >=0; )
              free ( currentConfig.variants[i] );
            free ( currentConfig.variants );
          }
          currentConfig.numLayouts = 
          currentConfig.numVariants = 1;
          currentConfig.layouts = malloc( sizeof ( char* ) );
          currentConfig.layouts[0] = strdup( layouts );
          currentConfig.variants = malloc( sizeof ( char* ) );
          currentConfig.variants[0] = strdup( "" );
        }

        if ( options != NULL )
        {
          if ( currentConfig.options != NULL ) 
          {
            for ( i = currentConfig.numOptions; --i >=0; )
              free ( currentConfig.options[i] );
            free ( currentConfig.options );
          }
          currentConfig.numOptions = 1;
          currentConfig.options = malloc( sizeof ( char* ) );
          currentConfig.options[0] = strdup( options );
        }

        XklDebug( 0, "New config:\n" );
        XklConfigDump( stdout, &currentConfig );
        if ( XklConfigActivate( &currentConfig ) )
            XklDebug( 0, "Set the config\n" );
        else
            XklDebug( 0, "Could not set the config: %s\n", XklGetLastError() );
        break;
      case ACTION_WRITE:
        XklConfigWriteFile( binary ? ( PACKAGE ".xkm" ) : ( PACKAGE ".xkb" ),
                            &currentConfig, 
                            binary );     
        XklDebug( 0, "The file " PACKAGE "%s is written\n", 
                  binary ? ".xkm" : ".xkb"  );
        break;
    }

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
