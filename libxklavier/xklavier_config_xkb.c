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

static void _XklApplyFun2XkbDesc( XkbDescPtr xkb, XkbDescModifierFunc fun,
                                  void *userData, Bool activeInServer )
{
  int mask;
  // XklDumpXkbDesc( "comp.xkb", xkb );
  if( fun == NULL )
    return;

  if( activeInServer )
  {
    mask = ( *fun ) ( NULL, NULL );
    if( mask == 0 )
      return;
    XkbGetUpdatedMap( _xklDpy, mask, xkb );
    // XklDumpXkbDesc( "restored1.xkb", xkb );
  }

  mask = ( *fun ) ( xkb, userData );
  if( activeInServer )
  {
    // XklDumpXkbDesc( "comp+.xkb", xkb );
    XkbSetMap( _xklDpy, mask, xkb );
    XSync( _xklDpy, False );

    // XkbGetUpdatedMap( _xklDpy, XkbAllMapComponentsMask, xkb );
    // XklDumpXkbDesc( "restored2.xkb", xkb );
  }
}

Bool XklMultipleLayoutsSupported( void )
{
  struct stat buf;
  return 0 == stat( MULTIPLE_LAYOUTS_CHECK_PATH, &buf );
}

Bool XklConfigActivate( const XklConfigRecPtr data, XkbDescModifierFunc fun,
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
      _XklApplyFun2XkbDesc( xkb, fun, userData, True );
#if 0
      XklDumpXkbDesc( "config.xkb", xkb );
#endif

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

int XklSetKeyAsSwitcher( XkbDescPtr kbd, void *userData )
{
#ifdef XKB_HEADERS_PRESENT
  if( kbd != NULL )
  {
    XkbClientMapPtr map = kbd->map;
    if( map != NULL )
    {
      KeySym keysym = ( KeySym ) userData;
      KeySym *psym = map->syms;
      int symno;

      for( symno = map->num_syms; --symno >= 0; psym++ )
      {
        if( *psym == keysym )
        {
          XklDebug( 160, "Changing %s to %s at %d\n",
                    XKeysymToString( *psym ),
                    XKeysymToString( XK_ISO_Next_Group ), psym - map->syms );
          *psym = XK_ISO_Next_Group;
          break;
        }
      }
    } else
      XklDebug( 160, "No client map in the keyboard description?\n" );
  }
  return XkbKeySymsMask | XkbKeyTypesMask | XkbKeyActionsMask;
#else
  return 0;
#endif
}

Bool XklConfigWriteXKMFile( const char *fileName, const XklConfigRecPtr data,
                            XkbDescModifierFunc fun, void *userData )
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
      _XklApplyFun2XkbDesc( xkb, fun, userData, False );

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
