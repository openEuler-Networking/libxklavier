#include <errno.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"

#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>

#define RULES_FILE "xfree86"

#define RULES_PATH ( XKB_BASE "/rules/" RULES_FILE )

#define XML_CFG_PATH ( XKB_BASE "/rules/xfree86.xml" )

// For "bad" X servers we hold our own copy
#define XML_CFG_FALLBACK_PATH ( DATA_DIR "/xfree86.xml" )

#define MULTIPLE_LAYOUTS_CHECK_PATH ( XKB_BASE "/symbols/pc/en_US" )

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

typedef struct _XklConfigRegistry
{
  xmlDocPtr doc;
  xmlXPathContextPtr xpathContext;
}
XklConfigRegistry;

XkbRF_VarDefsRec _xklVarDefs;

static XklConfigRegistry theRegistry;

static xmlXPathCompExprPtr modelsXPath;
static xmlXPathCompExprPtr layoutsXPath;
static xmlXPathCompExprPtr optionGroupsXPath;

static XkbRF_RulesPtr rules;
static XkbComponentNamesRec componentNames;
static char *locale;

#define _XklConfigRegistryIsInitialized() \
  ( theRegistry.xpathContext != NULL )

static xmlChar *_XklNodeGetXmlLangAttr( xmlNodePtr nptr )
{
  if( nptr->properties != NULL &&
      !strcmp( "lang", nptr->properties[0].name ) &&
      nptr->properties[0].ns != NULL &&
      !strcmp( "xml", nptr->properties[0].ns->prefix ) &&
      nptr->properties[0].children != NULL )
    return nptr->properties[0].children->content;
  else
    return NULL;
}

static Bool _XklReadConfigItem( xmlNodePtr iptr, XklConfigItemPtr pci )
{
  xmlNodePtr nameElement, descElement = NULL, ntDescElement =
    NULL, nptr, ptr, shortDescElement = NULL, ntShortDescElement = NULL;
  int maxDescPriority = -1;
  int maxShortDescPriority = -1;

  *pci->name = 0;
  *pci->shortDescription = 0;
  *pci->description = 0;
  if( iptr->type != XML_ELEMENT_NODE )
    return False;
  ptr = iptr->children;
  while( ptr != NULL )
  {
    switch ( ptr->type )
    {
      case XML_ELEMENT_NODE:
        if( !strcmp( ptr->name, "configItem" ) )
          break;
        return False;
      case XML_TEXT_NODE:
        ptr = ptr->next;
        continue;
      default:
        return False;
    }
    break;
  }
  if( ptr == NULL )
    return False;

  nptr = ptr->children;

  if( nptr->type == XML_TEXT_NODE )
    nptr = nptr->next;
  nameElement = nptr;
  nptr = nptr->next;

  while( nptr != NULL )
  {
    if( nptr->type != XML_TEXT_NODE )
    {
      xmlChar *lang = _XklNodeGetXmlLangAttr( nptr );

      if( lang != NULL )
      {
        int priority = _XklGetLanguagePriority( lang );
        if( !strcmp( nptr->name, "description" ) && ( priority > maxDescPriority ) )    // higher priority
        {
          descElement = nptr;
          maxDescPriority = priority;
        } else if( !strcmp( nptr->name, "shortDescription" ) && ( priority > maxShortDescPriority ) )   // higher priority
        {
          shortDescElement = nptr;
          maxShortDescPriority = priority;
        }
      } else
      {
        if( !strcmp( nptr->name, "description" ) )
          ntDescElement = nptr;
        else if( !strcmp( nptr->name, "shortDescription" ) )
          ntShortDescElement = nptr;
      }
    }
    nptr = nptr->next;
  }

  // if no language-specific description found - use the ones without lang
  if( descElement == NULL )
    descElement = ntDescElement;

  if( shortDescElement == NULL )
    shortDescElement = ntShortDescElement;

  //
  // Actually, here we should have some code to find the correct localized description...
  // 

  if( nameElement != NULL && nameElement->children != NULL )
    strncat( pci->name, nameElement->children->content,
             XKL_MAX_CI_NAME_LENGTH - 1 );

  if( shortDescElement != NULL && shortDescElement->children != NULL )
    strncat( pci->shortDescription,
             _XklLocaleFromUtf8( shortDescElement->children->content ),
             XKL_MAX_CI_SHORT_DESC_LENGTH - 1 );

  if( descElement != NULL && descElement->children != NULL )
    strncat( pci->description,
             _XklLocaleFromUtf8( descElement->children->content ),
             XKL_MAX_CI_DESC_LENGTH - 1 );
  return True;
}

static void _XklConfigEnumFromNodeSet( xmlNodeSetPtr nodes,
                                       ConfigItemProcessFunc func,
                                       void *userData )
{
  int i;
  if( nodes != NULL )
  {
    xmlNodePtr *theNodePtr = nodes->nodeTab;
    for( i = nodes->nodeNr; --i >= 0; )
    {
      XklConfigItem ci;
      if( _XklReadConfigItem( *theNodePtr, &ci ) )
        func( &ci, userData );

      theNodePtr++;
    }
  }
}

static void _XklConfigEnumSimple( xmlXPathCompExprPtr xpathCompExpr,
                                  ConfigItemProcessFunc func, void *userData )
{
  xmlXPathObjectPtr xpathObj;

  if( !_XklConfigRegistryIsInitialized(  ) )
    return;
  xpathObj = xmlXPathCompiledEval( xpathCompExpr, theRegistry.xpathContext );
  if( xpathObj != NULL )
  {
    _XklConfigEnumFromNodeSet( xpathObj->nodesetval, func, userData );
    xmlXPathFreeObject( xpathObj );
  }
}

static void _XklConfigEnumDirect( const char *format,
                                  const char *value,
                                  ConfigItemProcessFunc func, void *userData )
{
  char xpathExpr[1024];
  xmlXPathObjectPtr xpathObj;

  if( !_XklConfigRegistryIsInitialized(  ) )
    return;
  snprintf( xpathExpr, sizeof xpathExpr, format, value );
  xpathObj = xmlXPathEval( xpathExpr, theRegistry.xpathContext );
  if( xpathObj != NULL )
  {
    _XklConfigEnumFromNodeSet( xpathObj->nodesetval, func, userData );
    xmlXPathFreeObject( xpathObj );
  }
}

static Bool _XklConfigFindObject( const char *format,
                                  const char *arg1,
                                  XklConfigItemPtr ptr /* in/out */ ,
                                  xmlNodePtr * nodePtr /* out */  )
{
  xmlXPathObjectPtr xpathObj;
  xmlNodeSetPtr nodes;
  Bool rv = False;
  char xpathExpr[1024];

  if( !_XklConfigRegistryIsInitialized(  ) )
    return False;

  snprintf( xpathExpr, sizeof xpathExpr, format, arg1, ptr->name );
  xpathObj = xmlXPathEval( xpathExpr, theRegistry.xpathContext );
  if( xpathObj == NULL )
    return False;

  nodes = xpathObj->nodesetval;
  if( nodes != NULL && nodes->nodeTab != NULL )
  {
    rv = _XklReadConfigItem( *nodes->nodeTab, ptr );
    if( nodePtr != NULL )
    {
      *nodePtr = *nodes->nodeTab;
    }
  }

  xmlXPathFreeObject( xpathObj );
  return rv;
}

char *_XklConfigRecMergeLayouts( const XklConfigRecPtr data )
{
  return _XklConfigRecMergeByComma( ( const char ** ) data->layouts,
                                    data->numLayouts );
}

char *_XklConfigRecMergeVariants( const XklConfigRecPtr data )
{
  return _XklConfigRecMergeByComma( ( const char ** ) data->variants,
                                    data->numVariants );
}

char *_XklConfigRecMergeOptions( const XklConfigRecPtr data )
{
  return _XklConfigRecMergeByComma( ( const char ** ) data->options,
                                    data->numOptions );
}

char *_XklConfigRecMergeByComma( const char **array, const int arrayLength )
{
  int len = 0;
  int i;
  char *merged;
  const char **theString;

  if( ( theString = array ) == NULL )
    return NULL;

  for( i = arrayLength; --i >= 0; theString++ )
  {
    if( *theString != NULL )
      len += strlen( *theString );
    len++;
  }

  if( len < 1 )
    return NULL;

  merged = ( char * ) malloc( len );
  merged[0] = '\0';

  theString = array;
  for( i = arrayLength; --i >= 0; theString++ )
  {
    if( *theString != NULL )
      strcat( merged, *theString );
    if( i != 0 )
      strcat( merged, "," );
  }
  return merged;
}

void _XklConfigRecSplitLayouts( XklConfigRecPtr data, const char *merged )
{
  _XklConfigRecSplitByComma( &data->layouts, &data->numLayouts, merged );
}

void _XklConfigRecSplitVariants( XklConfigRecPtr data, const char *merged )
{
  _XklConfigRecSplitByComma( &data->variants, &data->numVariants, merged );
}

void _XklConfigRecSplitOptions( XklConfigRecPtr data, const char *merged )
{
  _XklConfigRecSplitByComma( &data->options, &data->numOptions, merged );
}

void _XklConfigRecSplitByComma( char ***array,
                                int *arraySize, const char *merged )
{
  const char *pc = merged;
  char **ppc, *npc;
  *arraySize = 0;
  *array = NULL;

  if( merged == NULL || merged[0] == '\0' )
    return;

  // first count the elements 
  while( ( npc = strchr( pc, ',' ) ) != NULL )
  {
    ( *arraySize )++;
    pc = npc + 1;
  }
  ( *arraySize )++;

  if( ( *arraySize ) != 0 )
  {
    int len;
    *array = ( char ** ) malloc( ( sizeof( char * ) ) * ( *arraySize ) );

    ppc = *array;
    pc = merged;
    while( ( npc = strchr( pc, ',' ) ) != NULL )
    {
      int len = npc - pc;
      //*ppc = ( char * ) strndup( pc, len );
      *ppc = ( char * ) malloc( len + 1 );
      if ( *ppc != NULL )
      {
        strncpy( *ppc, pc, len );
        (*ppc)[len] = '\0';
      }

      ppc++;
      pc = npc + 1;
    }

    //len = npc - pc;
    len = strlen( pc );
    //*ppc = ( char * ) strndup( pc, len );
    *ppc = ( char * ) malloc( len + 1 );
    if ( *ppc != NULL )
      strcpy( *ppc, pc );
  }
}

static Bool _XklConfigPrepareBeforeKbd( const XklConfigRecPtr data )
{
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

  return True;
}

static void _XklConfigCleanAfterKbd(  )
{
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

void XklConfigInit( void )
{
  xmlXPathInit(  );
  modelsXPath = xmlXPathCompile( "/xkbConfigRegistry/modelList/model" );
  layoutsXPath = xmlXPathCompile( "/xkbConfigRegistry/layoutList/layout" );
  optionGroupsXPath =
    xmlXPathCompile( "/xkbConfigRegistry/optionList/group" );
  _XklI18NInit(  );
}

void XklConfigTerm( void )
{
  if( modelsXPath != NULL )
  {
    xmlXPathFreeCompExpr( modelsXPath );
    modelsXPath = NULL;
  }
  if( layoutsXPath != NULL )
  {
    xmlXPathFreeCompExpr( layoutsXPath );
    layoutsXPath = NULL;
  }
  if( optionGroupsXPath != NULL )
  {
    xmlXPathFreeCompExpr( optionGroupsXPath );
    optionGroupsXPath = NULL;
  }
}

Bool XklConfigLoadRegistry( void )
{
  struct stat statBuf;

  const char *fileName = XML_CFG_PATH;
  if( stat( XML_CFG_PATH, &statBuf ) != 0 )
    fileName = XML_CFG_FALLBACK_PATH;

  theRegistry.doc = xmlParseFile( fileName );
  if( theRegistry.doc == NULL )
  {
    theRegistry.xpathContext = NULL;
    _xklLastErrorMsg = "Could not parse XKB configuration registry";
  } else
    theRegistry.xpathContext = xmlXPathNewContext( theRegistry.doc );
  return _XklConfigRegistryIsInitialized(  );
}

void XklConfigFreeRegistry( void )
{
  if( _XklConfigRegistryIsInitialized(  ) )
  {
    xmlXPathFreeContext( theRegistry.xpathContext );
    xmlFreeDoc( theRegistry.doc );
    theRegistry.xpathContext = NULL;
    theRegistry.doc = NULL;
  }
}

void XklConfigEnumModels( ConfigItemProcessFunc func, void *userData )
{
  _XklConfigEnumSimple( modelsXPath, func, userData );
}

void XklConfigEnumLayouts( ConfigItemProcessFunc func, void *userData )
{
  _XklConfigEnumSimple( layoutsXPath, func, userData );
}

void XklConfigEnumLayoutVariants( const char *layoutName,
                                  ConfigItemProcessFunc func, void *userData )
{
  _XklConfigEnumDirect
    ( "/xkbConfigRegistry/layoutList/layout/variantList/variant[../../configItem/name = '%s']",
      layoutName, func, userData );
}

void XklConfigEnumOptionGroups( GroupProcessFunc func, void *userData )
{
  xmlXPathObjectPtr xpathObj;
  int i;

  if( !_XklConfigRegistryIsInitialized(  ) )
    return;
  xpathObj =
    xmlXPathCompiledEval( optionGroupsXPath, theRegistry.xpathContext );
  if( xpathObj != NULL )
  {
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    xmlNodePtr *theNodePtr = nodes->nodeTab;
    for( i = nodes->nodeNr; --i >= 0; )
    {
      XklConfigItem ci;

      if( _XklReadConfigItem( *theNodePtr, &ci ) )
      {
        Bool allowMC = True;
        xmlChar *allowMCS =
          xmlGetProp( *theNodePtr, "allowMultipleSelection" );
        if( allowMCS != NULL )
        {
          allowMC = strcmp( "false", allowMCS );
          xmlFree( allowMCS );
        }

        func( &ci, allowMC, userData );
      }

      theNodePtr++;
    }
    xmlXPathFreeObject( xpathObj );
  }
}

void XklConfigEnumOptions( const char *optionGroupName,
                           ConfigItemProcessFunc func, void *userData )
{
  _XklConfigEnumDirect
    ( "/xkbConfigRegistry/optionList/group/option[../configItem/name = '%s']",
      optionGroupName, func, userData );
}

Bool XklConfigFindModel( XklConfigItemPtr ptr /* in/out */  )
{
  return
    _XklConfigFindObject
    ( "/xkbConfigRegistry/modelList/model[configItem/name = '%s%s']", "",
      ptr, NULL );
}

Bool XklConfigFindLayout( XklConfigItemPtr ptr /* in/out */  )
{
  return
    _XklConfigFindObject
    ( "/xkbConfigRegistry/layoutList/layout[configItem/name = '%s%s']", "",
      ptr, NULL );
}

Bool XklConfigFindVariant( const char *layoutName,
                           XklConfigItemPtr ptr /* in/out */  )
{
  return
    _XklConfigFindObject
    ( "/xkbConfigRegistry/layoutList/layout/variantList/variant"
      "[../../configItem/name = '%s' and configItem/name = '%s']",
      layoutName, ptr, NULL );
}

Bool XklConfigFindOptionGroup( XklConfigItemPtr ptr /* in/out */ ,
                               Bool * allowMultipleSelection /* out */  )
{
  xmlNodePtr node;
  Bool rv =
    _XklConfigFindObject
    ( "/xkbConfigRegistry/optionList/group[configItem/name = '%s%s']", "",
      ptr, &node );

  if( rv && allowMultipleSelection != NULL )
  {
    xmlChar *val = xmlGetProp( node, "allowMultipleSelection" );
    *allowMultipleSelection = False;
    if( val != NULL )
    {
      *allowMultipleSelection = !strcmp( val, "true" );
      xmlFree( val );
    }
  }
  return rv;
}

Bool XklConfigFindOption( const char *optionGroupName,
                          XklConfigItemPtr ptr /* in/out */  )
{
  return
    _XklConfigFindObject
    ( "/xkbConfigRegistry/optionList/group/option"
      "[../configItem/name = '%s' and configItem/name = '%s']",
      optionGroupName, ptr, NULL );
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
  return rv;
}

int XklSetKeyAsSwitcher( XkbDescPtr kbd, void *userData )
{
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
}

Bool XklConfigWriteXKMFile( const char *fileName, const XklConfigRecPtr data,
                            XkbDescModifierFunc fun, void *userData )
{
  Bool rv = False;

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
  return rv;
}
