#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

#define SHORTCUT_OPTION_PREFIX "grp:"

char* currentXmmRules = NULL;

XklConfigRec currentXmmConfig;

Atom xmmStateAtom;

const char **_XklXmmGetGroupNames( void )
{
  return (const char **)currentXmmConfig.layouts;
}

void _XklXmmGrabShortcuts( )
{
  int i;
  XmmShortcutPtr shortcut;
  const XmmSwitchOptionPtr option = _XklXmmGetCurrentShortcut();
  
  XklDebug( 150, "Found shortcut option: %p\n", option );
  if( option == NULL )
    return;
  
  shortcut = option->shortcuts;
  for( i = option->numShortcuts; --i >= 0; shortcut++ )
  {
    int keycode = XKeysymToKeycode( _xklDpy, shortcut->keysym );
    _XklXmmGrabIgnoringIndicators( keycode, 
                                   shortcut->modifiers );
  }
}

void _XklXmmUngrabShortcuts( )
{
  int i;
  XmmShortcutPtr shortcut;
  const XmmSwitchOptionPtr option = _XklXmmGetCurrentShortcut();
  
  if( option == NULL )
    return;
  
  shortcut = option->shortcuts;
  for( i = option->numShortcuts; --i >= 0; shortcut++ )
  {
    int keycode = XKeysymToKeycode( _xklDpy, shortcut->keysym );
    _XklXmmUngrabIgnoringIndicators( keycode, 
                                     shortcut->modifiers );
  }
}

const XmmSwitchOptionPtr _XklXmmGetCurrentShortcut()
{
  const char* optionName = _XklXmmGetCurrentShortcutOptionName();
  XklDebug( 150, "Configured switch option: [%s]\n", optionName );
  if( optionName == NULL )
    return NULL;
  XmmSwitchOptionPtr switchOption = allSwitchOptions;
  while( switchOption->optionName != NULL )
  {
    if( !strcmp( switchOption->optionName, optionName ) )
      return switchOption;
    switchOption++;
  }
  return NULL;
}

const char* _XklXmmGetCurrentShortcutOptionName( )
{
  int i;
  char** option = currentXmmConfig.options;
  for( i = currentXmmConfig.numOptions; --i >= 0; option++ )
  {
    /* starts with "grp:" */
    if( strstr( *option, SHORTCUT_OPTION_PREFIX ) != NULL )
    {
      return *option + sizeof SHORTCUT_OPTION_PREFIX - 1;
    }
  }
  return NULL;
}

const XmmSwitchOptionPtr _XklXmmFindSwitchOption( unsigned keycode, 
                                                  unsigned state, 
                                                  int* currentShortcut_rv )
{
  const XmmSwitchOptionPtr rv = _XklXmmGetCurrentShortcut();
  int i;
  
  if( rv != NULL )
  {
    XmmShortcutPtr sc = rv->shortcuts;
    for( i=rv->numShortcuts; --i>=0; sc++ )
    {
      if( ( XKeysymToKeycode( _xklDpy, sc->keysym ) == keycode ) &&
          ( ( state & sc->modifiers ) == sc->modifiers ) )
      {
        return rv;
      }
    }
  }
  return NULL;
}

int _XklXmmResumeListen(  )
{
  if( _xklListenerType & XKLL_MANAGE_LAYOUTS )
    _XklXmmGrabShortcuts();
  return 0;
}

int _XklXmmPauseListen(  )
{
  if( _xklListenerType & XKLL_MANAGE_LAYOUTS )
    _XklXmmUngrabShortcuts();
  return 0;
}

unsigned _XklXmmGetNumGroups( void )
{
  return currentXmmConfig.numLayouts;
}
  
void _XklXmmFreeAllInfo(  )
{
  if( currentXmmRules != NULL )
  {
    free( currentXmmRules );
    currentXmmRules = NULL;
  }
  XklConfigRecReset( &currentXmmConfig );
}

Bool _XklXmmLoadAllInfo(  )
{
  return _XklConfigGetFullFromServer( &currentXmmRules, &currentXmmConfig );
}

void _XklXmmGetRealState( XklState * state )
{
  unsigned char *propval = NULL;
  Atom actualType;
  int actualFormat;
  unsigned long bytesRemaining;
  unsigned long actualItems;
  int result;
  
  memset( state, 0, sizeof( *state ) );

  result = XGetWindowProperty( _xklDpy, _xklRootWindow, xmmStateAtom, 0L, 1L, 
                               False, XA_INTEGER, &actualType, &actualFormat,
                               &actualItems, &bytesRemaining, 
                               &propval );
  
  if( Success != result )
  {
    XklDebug( 160, "Could not get the xmodmap current group: %d\n", result );
    return;
  }

  if( actualFormat != 32 ||
      actualItems != 1 )
  {  
    XklDebug( 160, "Could not get the xmodmap current group\n" );
    return;
  }
  
  state->group = *(CARD32*)propval;
  XFree( propval );
  state->indicators = 0;  
}

void _XklXmmActualizeGroup( int group )
{
  char cmd[1024];
  int res;
  const char* layoutName = NULL;
  
  if( currentXmmConfig.numLayouts < group )
    return;
  
  layoutName = currentXmmConfig.layouts[group];
  
  snprintf( cmd, sizeof cmd, 
            "xmodmap %s/xmodmap.%s",
            XMODMAP_BASE, layoutName );

  res = system( cmd );
  if( res > 0 )
  {
    XklDebug( 0, "xmodmap error %d\n", res );
  } else if( res < 0 )
  {
    XklDebug( 0, "Could not execute xmodmap: %d\n", res );
  }
  XSync( _xklDpy, False );
}

void _XklXmmLockGroup( int group )
{
  CARD32 propval;

  if( currentXmmConfig.numLayouts < group )
    return;
  
  /* updating the status property */
  propval = group;
  XChangeProperty( _xklDpy, _xklRootWindow, xmmStateAtom, 
                   XA_INTEGER, 32, PropModeReplace, 
                   (unsigned char*)&propval, 1 );
  XSync( _xklDpy, False );
}

int _XklXmmInit( void )
{
  static XklVTable xklXmmVTable =
  {
    "xmodmap",
    XKLF_MULTIPLE_LAYOUTS_SUPPORTED |
      XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT, 
    _XklXmmConfigActivate,
    _XklXmmConfigInit,
    _XklXmmConfigLoadRegistry,
    NULL,
    _XklXmmEventHandler,
    _XklXmmFreeAllInfo,
    _XklXmmGetGroupNames,
    _XklXmmGetNumGroups,
    _XklXmmGetRealState,
    _XklXmmLoadAllInfo,
    _XklXmmLockGroup,
    _XklXmmPauseListen,
    _XklXmmResumeListen,
    NULL,
  };

  xklXmmVTable.baseConfigAtom =
    XInternAtom( _xklDpy, "_XMM_NAMES", False );
  xklXmmVTable.backupConfigAtom =
    XInternAtom( _xklDpy, "_XMM_NAMES_BACKUP", False );

  xmmStateAtom =
    XInternAtom( _xklDpy, "_XMM_STATE", False );
  
  xklXmmVTable.defaultModel = "generic";
  xklXmmVTable.defaultLayout = "us";

  xklVTable = &xklXmmVTable;

  XklDebug( 0, "XMM backend inited\n" );
  return 0;
}
