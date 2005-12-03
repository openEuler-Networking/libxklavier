#include <errno.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <fcntl.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

#ifdef XKB_HEADERS_PRESENT
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>
#endif

/* For "bad" X servers we hold our own copy */
#define XML_CFG_FALLBACK_PATH ( DATA_DIR "/xfree86.xml" )

#define XKBCOMP ( XKB_BASE "/xkbcomp" )

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#ifdef XKB_HEADERS_PRESENT
static XkbRF_RulesPtr _xklRules;

static XkbRF_RulesPtr _XklLoadRulesSet( void )
{
  XkbRF_RulesPtr rulesSet = NULL;
  char fileName[MAXPATHLEN] = "";
  char *rf = _XklGetRulesSetName( XKB_DEFAULT_RULESET );
  char *locale = NULL;

  if( rf == NULL )
  {
    _xklLastErrorMsg = "Could not find the XKB rules set";
    return NULL;
  }

  locale = setlocale( LC_ALL, NULL );

  snprintf( fileName, sizeof fileName, XKB_BASE "/rules/%s", rf );
  XklDebug( 160, "Loading rules from [%s]\n", fileName );

  rulesSet = XkbRF_Load( fileName, locale, True, True );

  if( rulesSet == NULL )
  {
    _xklLastErrorMsg = "Could not load rules";
    return NULL;
  }
  return rulesSet;
}

static void _XklFreeRulesSet( void )
{
  if ( _xklRules )
    XkbRF_Free( _xklRules, True );
  _xklRules = NULL;
}
#endif

void _XklXkbConfigInit( void )
{
#ifdef XKB_HEADERS_PRESENT
  XkbInitAtoms( NULL );
#endif
}

Bool _XklXkbConfigLoadRegistry( void )
{
  struct stat statBuf;
  char fileName[MAXPATHLEN] = "";
  char* rf = _XklGetRulesSetName( XKB_DEFAULT_RULESET );

  if ( rf == NULL )
    return False;

  snprintf( fileName, sizeof fileName, XKB_BASE "/rules/%s.xml", rf );

  if( stat( fileName, &statBuf ) != 0 )
  {
    strncpy( fileName, XML_CFG_FALLBACK_PATH, sizeof fileName );
    fileName[ MAXPATHLEN - 1 ] = '\0';
  }

  return XklConfigLoadRegistryFromFile( fileName );
}

#ifdef XKB_HEADERS_PRESENT
Bool _XklXkbConfigPrepareNative( const XklConfigRecPtr data, XkbComponentNamesPtr componentNamesPtr )
{
  XkbRF_VarDefsRec _xklVarDefs;
  Bool gotComponents;

  memset( &_xklVarDefs, 0, sizeof( _xklVarDefs ) );

  _xklRules = _XklLoadRulesSet();
  if( !_xklRules )
  {
    return False;
  }

  _xklVarDefs.model = ( char * ) data->model;

  if( data->layouts != NULL )
    _xklVarDefs.layout = _XklConfigRecMergeLayouts( data );

  if( data->variants != NULL )
    _xklVarDefs.variant = _XklConfigRecMergeVariants( data );

  if( data->options != NULL )
    _xklVarDefs.options = _XklConfigRecMergeOptions( data );

  gotComponents = XkbRF_GetComponents( _xklRules, &_xklVarDefs, componentNamesPtr );

  free( _xklVarDefs.layout );
  free( _xklVarDefs.variant );
  free( _xklVarDefs.options );

  if( !gotComponents )
  {
    _xklLastErrorMsg = "Could not translate rules into components";
    /* Just cleanup the stuff in case of failure */
    _XklXkbConfigCleanupNative( componentNamesPtr );
    
    return False;
  }

  if ( _xklDebugLevel >= 200 )
  {
    XklDebug( 200, "keymap: %s\n", componentNamesPtr->keymap );
    XklDebug( 200, "keycodes: %s\n", componentNamesPtr->keycodes );
    XklDebug( 200, "compat: %s\n", componentNamesPtr->compat );
    XklDebug( 200, "types: %s\n", componentNamesPtr->types );
    XklDebug( 200, "symbols: %s\n", componentNamesPtr->symbols );
    XklDebug( 200, "geometry: %s\n", componentNamesPtr->geometry );
  }
  return True;
}

void _XklXkbConfigCleanupNative( XkbComponentNamesPtr componentNamesPtr )
{
  _XklFreeRulesSet();

  free(componentNamesPtr->keymap);
  free(componentNamesPtr->keycodes);
  free(componentNamesPtr->compat);
  free(componentNamesPtr->types);
  free(componentNamesPtr->symbols);
  free(componentNamesPtr->geometry);
}

static XkbDescPtr _XklConfigGetKeyboard( XkbComponentNamesPtr componentNamesPtr, Bool activate )
{
  XkbDescPtr xkb = NULL;
#if 0
  xkb = XkbGetKeyboardByName( _xklDpy, 
                              XkbUseCoreKbd, 
                              &componentNames,
                              XkbGBN_AllComponentsMask &
                              ( ~XkbGBN_GeometryMask ), 
                              XkbGBN_AllComponentsMask &
                              ( ~XkbGBN_GeometryMask ), 
                              activate );
#else
  char xkmFN[L_tmpnam];
  char xkbFN[L_tmpnam];
  FILE* tmpxkm;
  XkbFileInfo result;
  int xkmloadres;

  if ( tmpnam( xkmFN ) != NULL && 
       tmpnam( xkbFN ) != NULL )
  {
    pid_t cpid, pid;
    int status = 0;
    FILE *tmpxkb;

    XklDebug( 150, "tmp XKB/XKM file names: [%s]/[%s]\n", xkbFN, xkmFN );
    if( (tmpxkb = fopen( xkbFN, "w" )) != NULL )
    {
      fprintf( tmpxkb, "xkb_keymap {\n" );
      fprintf( tmpxkb, "        xkb_keycodes  { include \"%s\" };\n", componentNamesPtr->keycodes );
      fprintf( tmpxkb, "        xkb_types     { include \"%s\" };\n", componentNamesPtr->types );
      fprintf( tmpxkb, "        xkb_compat    { include \"%s\" };\n", componentNamesPtr->compat );
      fprintf( tmpxkb, "        xkb_symbols   { include \"%s\" };\n", componentNamesPtr->symbols );
      fprintf( tmpxkb, "        xkb_geometry  { include \"%s\" };\n", componentNamesPtr->geometry );
      fprintf( tmpxkb, "};\n" );
      fclose( tmpxkb );
    
      XklDebug( 150, "xkb_keymap {\n"
        "        xkb_keycodes  { include \"%s\" };\n"
        "        xkb_types     { include \"%s\" };\n"
        "        xkb_compat    { include \"%s\" };\n"
        "        xkb_symbols   { include \"%s\" };\n"
        "        xkb_geometry  { include \"%s\" };\n};\n", 
        componentNamesPtr->keycodes,
        componentNamesPtr->types,
        componentNamesPtr->compat,
        componentNamesPtr->symbols,
        componentNamesPtr->geometry );
       
      cpid=fork();
      switch( cpid )
      {
        case -1:
          XklDebug( 0, "Could not fork: %d\n", errno );
          break;
        case 0:
          /* child */
          XklDebug( 160, "Executing %s\n", XKBCOMP );
          XklDebug( 160, "%s %s %s %s %s %s %s\n",
            XKBCOMP, XKBCOMP, "-I", "-I" XKB_BASE, "-xkm", xkbFN, xkmFN );
          execl( XKBCOMP, XKBCOMP, "-I", "-I" XKB_BASE, "-xkm", xkbFN, xkmFN, NULL );
          XklDebug( 0, "Could not exec %s: %d\n", XKBCOMP, errno );
          exit( 1 );
        default:
          /* parent */
          pid = waitpid( cpid, &status, 0 );
          XklDebug( 150, "Return status of %d (well, started %d): %d\n", pid, cpid, status );
          memset( (char *)&result, 0, sizeof(result) );
          result.xkb = XkbAllocKeyboard();

          if( Success == XkbChangeKbdDisplay( _xklDpy, &result ) )
          {
            XklDebug( 150, "Hacked the kbddesc - set the display...\n" );
            if( (tmpxkm = fopen( xkmFN, "r" )) != NULL )
            {
              xkmloadres = XkmReadFile( tmpxkm, XkmKeymapLegal, XkmKeymapLegal, &result);
              XklDebug( 150, "Loaded %s output as XKM file, got %d (comparing to %d)\n", 
                        XKBCOMP, (int)xkmloadres, (int)XkmKeymapLegal );
              if ( (int)xkmloadres != (int)XkmKeymapLegal )
              {
                XklDebug( 150, "Loaded legal keymap\n" );
                if( activate )
                {
                  XklDebug( 150, "Activating it...\n" );
                  if( XkbWriteToServer(&result) )
                  {
                     XklDebug( 150, "Updating the keyboard...\n" );
                     xkb = result.xkb;
                  } else
                  {
                     XklDebug( 0, "Could not write keyboard description to the server\n" );
                  }
                } else /* no activate, just load */
                  xkb = result.xkb;
              } else /* could not load properly */
              {
                XklDebug( 0, "Could not load %s output as XKM file, got %d (asked %d)\n", 
                             XKBCOMP, (int)xkmloadres, (int)XkmKeymapLegal );
              }
              fclose( tmpxkm );
              XklDebug( 160, "Unlinking the temporary xkm file %s\n", xkmFN );
              if ( _xklDebugLevel < 500 ) /* don't remove on high debug levels! */
              {
                if ( remove( xkmFN ) == -1 )
                  XklDebug( 0, "Could not unlink the temporary xkm file %s: %d\n", 
                               xkmFN, errno );
              } else
                XklDebug( 500, "Well, not really - the debug level is too high: %d\n", _xklDebugLevel );
            } else /* could not open the file */
            {
              XklDebug( 0, "Could not open the temporary xkm file %s\n", xkmFN );
            }
          } else /* could not assign to display */
          {
            XklDebug( 0, "Could not change the keyboard description to display\n" );
          } 
          if ( xkb == NULL )
            XkbFreeKeyboard( result.xkb, XkbAllComponentsMask, True );
          break;
      }
      XklDebug( 160, "Unlinking the temporary xkb file %s\n", xkbFN );
      if ( _xklDebugLevel < 500 ) /* don't remove on high debug levels! */
      {
        if ( remove( xkbFN ) == -1 )
          XklDebug( 0, "Could not unlink the temporary xkb file %s: %d\n", 
                    xkbFN, errno );
      } else
        XklDebug( 500, "Well, not really - the debug level is too high: %d\n", _xklDebugLevel );
    } else /* could not open input tmp file */
    {
      XklDebug( 0, "Could not open tmp XKB file [%s]: %d\n", xkbFN, errno );
    }
  } else
  {
    XklDebug( 0, "Could not get tmp names\n" );
  }

#endif
  return xkb;
}
#else /* no XKB headers */
Bool _XklXkbConfigPrepareNative( const XklConfigRecPtr data, void * componentNamesPtr )
{
  return False;
}

void _XklXkbConfigCleanupNative( void * componentNamesPtr )
{
}
#endif

/* check only client side support */
Bool _XklXkbConfigMultipleLayoutsSupported( void )
{
  enum { NON_SUPPORTED, SUPPORTED, UNCHECKED };

  static int supportState = UNCHECKED;

  if( supportState == UNCHECKED )
  {
    XklConfigRec data;
    char *layouts[] = { "us", "de" };
    char *variants[] = { NULL, NULL };
#ifdef XKB_HEADERS_PRESENT
    XkbComponentNamesRec componentNames;
    memset( &componentNames, 0, sizeof( componentNames ) );
#endif

    data.model = "pc105"; 
    data.numVariants =
    data.numLayouts = 2;
    data.numOptions = 0;
    data.layouts = layouts;
    data.variants = variants;
    data.options = NULL;

    XklDebug( 100, "!!! Checking multiple layouts support\n" );
    supportState = NON_SUPPORTED;
#ifdef XKB_HEADERS_PRESENT
    if( _XklXkbConfigPrepareNative( &data, &componentNames ) )
    {
      XklDebug( 100, "!!! Multiple layouts ARE supported\n" );
      supportState = SUPPORTED;
      _XklXkbConfigCleanupNative( &componentNames );
    } else
    {
      XklDebug( 100, "!!! Multiple layouts ARE NOT supported\n" );
    }
#endif
  }
  return supportState == SUPPORTED;
}

Bool _XklXkbConfigActivate( const XklConfigRecPtr data )
{
  Bool rv = False;
#if 0
  {
  int i;
  XklDebug( 150, "New model: [%s]\n", data->model );
  XklDebug( 150, "New layouts: %p\n", data->layouts );
  for( i = data->numLayouts; --i >= 0; )
    XklDebug( 150, "New layout[%d]: [%s]\n", i, data->layouts[i] );
  XklDebug( 150, "New variants: %p\n", data->variants );
  for( i = data->numVariants; --i >= 0; )
    XklDebug( 150, "New variant[%d]: [%s]\n", i, data->variants[i] );
  XklDebug( 150, "New options: %p\n", data->options );
  for( i = data->numOptions; --i >= 0; )
    XklDebug( 150, "New option[%d]: [%s]\n", i, data->options[i] );
  }
#endif

#ifdef XKB_HEADERS_PRESENT
  XkbComponentNamesRec componentNames;
  memset( &componentNames, 0, sizeof( componentNames ) );

  if( _XklXkbConfigPrepareNative( data, &componentNames ) )
  {
    XkbDescPtr xkb;
    xkb = _XklConfigGetKeyboard( &componentNames, True );
    if( xkb != NULL )
    {
      if( XklSetNamesProp
          ( xklVTable->baseConfigAtom, _XklGetRulesSetName( XKB_DEFAULT_RULESET ), data ) )
          /* We do not need to check the result of _XklGetRulesSetName - 
             because PrepareBeforeKbd did it for us */
        rv = True;
      else
        _xklLastErrorMsg = "Could not set names property";
      XkbFreeKeyboard( xkb, XkbAllComponentsMask, True );
    } else
    {
      _xklLastErrorMsg = "Could not load keyboard description";
    }
    _XklXkbConfigCleanupNative( &componentNames );
  }
#endif
  return rv;
}

Bool _XklXkbConfigWriteFile( const char *fileName, 
                             const XklConfigRecPtr data,
                             const Bool binary )
{
  Bool rv = False;

#ifdef XKB_HEADERS_PRESENT
  XkbComponentNamesRec componentNames;
  FILE *output = fopen( fileName, "w" );
  XkbFileInfo dumpInfo;

  if( output == NULL )
  {
    _xklLastErrorMsg = "Could not open the XKB file";
    return False;
  }

  memset( &componentNames, 0, sizeof( componentNames ) );

  if( _XklXkbConfigPrepareNative( data, &componentNames ) )
  {
    XkbDescPtr xkb;
    xkb = _XklConfigGetKeyboard( &componentNames, False );
    if( xkb != NULL )
    {
      dumpInfo.defined = 0;
      dumpInfo.xkb = xkb;
      dumpInfo.type = XkmKeymapFile;
      if( binary )
        rv = XkbWriteXKMFile( output, &dumpInfo );
      else
        rv = XkbWriteXKBFile( output, &dumpInfo, True, NULL, NULL );

      XkbFreeKeyboard( xkb, XkbGBN_AllComponentsMask, True );
    } else
      _xklLastErrorMsg = "Could not load keyboard description";
    _XklXkbConfigCleanupNative( &componentNames );
  }
  fclose( output );
#endif
  return rv;
}
