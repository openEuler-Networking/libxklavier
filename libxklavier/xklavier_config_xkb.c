#include <errno.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"

#include "xklavier_private_xkb.h"

#ifdef XKB_HEADERS_PRESENT
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>
#endif

#define RULES_FILE "xfree86"

#define RULES_PATH ( XKB_BASE "/rules/" RULES_FILE )

#define XML_CFG_PATH ( XKB_BASE "/rules/xfree86.xml" )

// For "bad" X servers we hold our own copy
#define XML_CFG_FALLBACK_PATH ( DATA_DIR "/xfree86.xml" )

#define MULTIPLE_LAYOUTS_CHECK_PATH ( XKB_BASE "/symbols/pc/en_US" )

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#ifdef XKB_HEADERS_PRESENT
XkbRF_VarDefsRec _xklVarDefs;

static XkbRF_RulesPtr rules;
static XkbComponentNamesRec componentNames;
#endif

static char *locale;

Bool XklConfigLoadRegistry( void )
{
  struct stat statBuf;
                                                                                          
  const char *fileName = XML_CFG_PATH;
  if( stat( XML_CFG_PATH, &statBuf ) != 0 )
    fileName = XML_CFG_FALLBACK_PATH;
                                                                                          
  return XklConfigLoadRegistryFromFile( fileName );
}

static Bool _XklConfigPrepareBeforeKbd( const XklConfigRecPtr data )
{
#ifdef XKB_HEADERS_PRESENT
  memset( &_xklVarDefs, 0, sizeof( _xklVarDefs ) );

  _xklVarDefs.model = ( char * ) data->model;

  if( data->layouts != NULL )
    _xklVarDefs.layout = _XklConfigRecMergeLayouts( data );

  if( data->variants != NULL )
    _xklVarDefs.variant = _XklConfigRecMergeVariants( data );

  if( data->options != NULL )
    _xklVarDefs.options = _XklConfigRecMergeOptions( data );

  locale = setlocale( LC_ALL, NULL );
  if( locale != NULL )
    locale = strdup( locale );

  rules = XkbRF_Load( RULES_PATH, locale, True, True );

  if( rules == NULL )
  {
    _xklLastErrorMsg = "Could not load rules";
    return False;
  }

  if( !XkbRF_GetComponents( rules, &_xklVarDefs, &componentNames ) )
  {
    _xklLastErrorMsg = "Could not translate rules into components";
    return False;
  }
#endif
  return True;
}

static void _XklConfigCleanAfterKbd(  )
{
#ifdef XKB_HEADERS_PRESENT
  XkbRF_Free( rules, True );

  if( locale != NULL )
  {
    free( locale );
    locale = NULL;
  }
  if( _xklVarDefs.layout != NULL )
  {
    free( _xklVarDefs.layout );
    _xklVarDefs.layout = NULL;
  }
  if( _xklVarDefs.options != NULL )
  {
    free( _xklVarDefs.options );
    _xklVarDefs.options = NULL;
  }
#endif
}

Bool XklMultipleLayoutsSupported( void )
{
  struct stat buf;
  return 0 == stat( MULTIPLE_LAYOUTS_CHECK_PATH, &buf );
}

Bool XklConfigActivate( const XklConfigRecPtr data,
                        void *userData )
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
    xkb =
      XkbGetKeyboardByName( _xklDpy, XkbUseCoreKbd, &componentNames,
                            XkbGBN_AllComponentsMask,
                            XkbGBN_AllComponentsMask &
                            ( ~XkbGBN_GeometryMask ), True );

    //!! Do I need to free it anywhere?
    if( xkb != NULL )
    {
      if( XklSetNamesProp
          ( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], RULES_FILE, data ) )
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

Bool XklConfigWriteXKMFile( const char *fileName, const XklConfigRecPtr data,
                            void *userData )
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
    xkb =
      XkbGetKeyboardByName( _xklDpy, XkbUseCoreKbd, &componentNames,
                            XkbGBN_AllComponentsMask,
                            XkbGBN_AllComponentsMask &
                            ( ~XkbGBN_GeometryMask ), False );
    if( xkb != NULL )
    {
      dumpInfo.defined = 0;
      dumpInfo.xkb = xkb;
      dumpInfo.type = XkmKeymapFile;
      rv = XkbWriteXKMFile( output, &dumpInfo );
      XkbFreeKeyboard( xkb, XkbGBN_AllComponentsMask, True );
    } else
      _xklLastErrorMsg = "Could not load keyboard description";
  }
  _XklConfigCleanAfterKbd(  );
  fclose( output );
#endif
  return rv;
}
