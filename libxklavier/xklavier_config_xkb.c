#include <errno.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>

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

// For "bad" X servers we hold our own copy
#define XML_CFG_FALLBACK_PATH ( DATA_DIR "/xfree86.xml" )

#define XKBCOMP ( XKB_BASE "/xkbcomp" )

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#ifdef XKB_HEADERS_PRESENT
XkbRF_VarDefsRec _xklVarDefs;
static XkbRF_RulesPtr _xklRules;
static XkbComponentNamesRec componentNames;
#endif

static char *locale;

static char* _XklGetRulesSetName( void )
{
#ifdef XKB_HEADERS_PRESENT
  static char rulesSetName[_XKB_RF_NAMES_PROP_MAXLEN] = "";
  if ( !rulesSetName[0] )
  {
    char* rf = NULL;
    if( !XklGetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], &rf, NULL ) || ( rf == NULL ) )
    {
      strncpy( rulesSetName, XKB_DEFAULT_RULESET, sizeof rulesSetName );
      XklDebug( 100, "Using default rules set: [%s]\n", rulesSetName );
      return rulesSetName;
    }
    strncpy( rulesSetName, rf, sizeof rulesSetName );
    free( rf );
  }
  XklDebug( 100, "Rules set: [%s]\n", rulesSetName );
  return rulesSetName;
#else
  return NULL;
#endif
}

#ifdef XKB_HEADERS_PRESENT
static XkbRF_RulesPtr _XklLoadRulesSet( void )
{
  char fileName[MAXPATHLEN] = "";
  char* rf = _XklGetRulesSetName();

  _xklRules = NULL;
  if( rf == NULL )
  {
    _xklLastErrorMsg = "Could not find the XKB rules set";
    return NULL;
  }

  locale = setlocale( LC_ALL, NULL );
  if( locale != NULL )
    locale = strdup( locale );

  snprintf( fileName, sizeof fileName, XKB_BASE "/rules/%s", rf );
  XklDebug( 160, "Loading rules from [%s]\n", fileName );

  _xklRules = XkbRF_Load( fileName, locale, True, True );

  if( _xklRules == NULL )
  {
    _xklLastErrorMsg = "Could not load rules";
    return NULL;
  }
  return _xklRules;
}
#endif

static void _XklFreeRulesSet( void )
{
#ifdef XKB_HEADERS_PRESENT
  if ( _xklRules )
    XkbRF_Free( _xklRules, True );
#endif
}

void _XklConfigXkbInit( void )
{
  XkbInitAtoms( NULL );
}

Bool XklConfigLoadRegistry( void )
{
  struct stat statBuf;
  char fileName[MAXPATHLEN] = "";
  char* rf = _XklGetRulesSetName();

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

static Bool _XklConfigPrepareBeforeKbd( const XklConfigRecPtr data )
{
#ifdef XKB_HEADERS_PRESENT
  XkbRF_RulesPtr rulesPtr = _XklLoadRulesSet();

  memset( &_xklVarDefs, 0, sizeof( _xklVarDefs ) );

  if( !rulesPtr )
    return False;

  _xklVarDefs.model = ( char * ) data->model;

  if( data->layouts != NULL )
    _xklVarDefs.layout = _XklConfigRecMergeLayouts( data );

  if( data->variants != NULL )
    _xklVarDefs.variant = _XklConfigRecMergeVariants( data );

  if( data->options != NULL )
    _xklVarDefs.options = _XklConfigRecMergeOptions( data );

  if( !XkbRF_GetComponents( rulesPtr, &_xklVarDefs, &componentNames ) )
  {
    _xklLastErrorMsg = "Could not translate rules into components";
    return False;
  }

  if ( _xklDebugLevel >= 200 )
  {
    XklDebug( 200, "keymap: %s\n", componentNames.keymap );
    XklDebug( 200, "keycodes: %s\n", componentNames.keycodes );
    XklDebug( 200, "compat: %s\n", componentNames.compat );
    XklDebug( 200, "types: %s\n", componentNames.types );
    XklDebug( 200, "symbols: %s\n", componentNames.symbols );
    XklDebug( 200, "geometry: %s\n", componentNames.geometry );
  }
#endif
  return True;
}

static void _XklConfigCleanAfterKbd(  )
{
#ifdef XKB_HEADERS_PRESENT
  _XklFreeRulesSet();

  free( locale );
  locale = NULL;

  free( _xklVarDefs.layout );
  free( _xklVarDefs.variant );
  free( _xklVarDefs.options );
  memset( &_xklVarDefs, 0, sizeof( _xklVarDefs ) );

  free(componentNames.keymap);
  free(componentNames.keycodes);
  free(componentNames.compat);
  free(componentNames.types);
  free(componentNames.symbols);
  free(componentNames.geometry);
#endif
}

#ifdef XKB_HEADERS_PRESENT
static XkbDescPtr _XklConfigGetKeyboard( Bool activate )
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
      fprintf( tmpxkb, "        xkb_keycodes  { include \"%s\" };\n", componentNames.keycodes );
      fprintf( tmpxkb, "        xkb_types     { include \"%s\" };\n", componentNames.types );
      fprintf( tmpxkb, "        xkb_compat    { include \"%s\" };\n", componentNames.compat );
      fprintf( tmpxkb, "        xkb_symbols   { include \"%s\" };\n", componentNames.symbols );
      fprintf( tmpxkb, "        xkb_geometry  { include \"%s\" };\n", componentNames.geometry );
      fprintf( tmpxkb, "};\n" );
      fclose( tmpxkb );
    
      XklDebug( 150, "xkb_keymap {\n"
        "        xkb_keycodes  { include \"%s\" };\n"
        "        xkb_types     { include \"%s\" };\n"
        "        xkb_compat    { include \"%s\" };\n"
        "        xkb_symbols   { include \"%s\" };\n"
        "        xkb_geometry  { include \"%s\" };\n};\n", 
        componentNames.keycodes,
        componentNames.types,
        componentNames.compat,
        componentNames.symbols,
        componentNames.geometry );
       
      cpid=fork();
      switch( cpid )
      {
        case -1:
          XklDebug( 0, "Could not fork: %d\n", errno );
          break;
        case 0:
          /* child */
          XklDebug( 160, "Executing %s\n", XKBCOMP );
          execl( XKBCOMP, XKBCOMP, "-I", "-I" XKB_BASE, "-xkm", xkbFN, xkmFN, NULL );
          XklDebug( 0, "Could not exec %s: %d\n", XKBCOMP, errno );
          exit( 1 );
        default:
          /* parent */
          pid = wait( &status );
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
              if ( remove( xkmFN ) == -1 )
                XklDebug( 0, "Could not unlink the temporary xkm file %s: %d\n", 
                             xkmFN, errno );
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
      if ( remove( xkbFN ) == -1 )
        XklDebug( 0, "Could not unlink the temporary xkb file %s: %d\n", 
                  xkbFN, errno );
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
#endif

// check only client side support
Bool XklMultipleLayoutsSupported( void )
{
  enum { NON_SUPPORTED, SUPPORTED, UNCHECKED };

  static int supportState = UNCHECKED;

  if( supportState == UNCHECKED )
  {
#ifdef XKB_HEADERS_PRESENT
    XkbRF_RulesPtr rulesPtr;
#endif
    XklDebug( 100, "!!! Checking multiple layouts support\n" );
    supportState = NON_SUPPORTED;
#ifdef XKB_HEADERS_PRESENT
    rulesPtr = _XklLoadRulesSet();
    if( rulesPtr )
    {
      XkbRF_VarDefsRec varDefs;
      XkbComponentNamesRec cNames;
      memset( &varDefs, 0, sizeof( varDefs ) );

      varDefs.model = "pc105";
      varDefs.layout = "a,b";
      varDefs.variant = "";
      varDefs.options = "";

      if( XkbRF_GetComponents( rulesPtr, &varDefs, &cNames ) )
      {
        XklDebug( 100, "!!! Multiple layouts ARE supported\n" );
        supportState = SUPPORTED;
      } else
      {
        XklDebug( 100, "!!! Multiple layouts ARE NOT supported\n" );
      }
      free(cNames.keymap);
      free(cNames.keycodes);
      free(cNames.compat);
      free(cNames.types);
      free(cNames.symbols);
      free(cNames.geometry);

      _XklFreeRulesSet();
    }
#endif
  }
  return supportState == SUPPORTED;
}

Bool XklConfigActivate( const XklConfigRecPtr data )
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
  if( _XklConfigPrepareBeforeKbd( data ) )
  {
    XkbDescPtr xkb;
    xkb = _XklConfigGetKeyboard( True );
    if( xkb != NULL )
    {
      if( XklSetNamesProp
          ( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], _XklGetRulesSetName(), data ) )
          // We do not need to check the result of _XklGetRulesSetName - 
          // because PrepareBeforeKbd did it for us
        rv = True;
      else
        _xklLastErrorMsg = "Could not set names property";
      XkbFreeKeyboard( xkb, XkbAllComponentsMask, True );
    } else
    {
      _xklLastErrorMsg = "Could not load keyboard description";
    }
  }
  _XklConfigCleanAfterKbd(  );
#endif
  return rv;
}

Bool XklConfigWriteFile( const char *fileName, 
                         const XklConfigRecPtr data,
                         const Bool binary )
{
  Bool rv = False;

#ifdef XKB_HEADERS_PRESENT
  FILE *output = fopen( fileName, "w" );
  XkbFileInfo dumpInfo;

  if( output == NULL )
  {
    _xklLastErrorMsg = "Could not open the XKB file";
    return False;
  }

  if( _XklConfigPrepareBeforeKbd( data ) )
  {
    XkbDescPtr xkb;
    xkb = _XklConfigGetKeyboard( False );
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
  }
  _XklConfigCleanAfterKbd(  );
  fclose( output );
#endif
  return rv;
}
