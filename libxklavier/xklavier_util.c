#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

XklState *XklGetCurrentState(  )
{
  return &_xklCurState;
}

const char *XklGetLastError(  )
{
  return _xklLastErrorMsg;
}

char *XklGetWindowTitle( Window w )
{
  Atom type_ret;
  int format_ret;
  unsigned long nitems, rest;
  unsigned char *prop;

  if( Success == XGetWindowProperty( _xklDpy, w, _xklAtoms[WM_NAME], 0L,
                                     -1L, False, XA_STRING, &type_ret,
                                     &format_ret, &nitems, &rest, &prop ) )
    return prop;
  else
    return NULL;
}

void XklLockGroup( int group )
{
  XklDebug( 100, "Posted request for change the group to %d ##\n", group );
  XkbLockGroup( _xklDpy, XkbUseCoreKbd, group );
  XSync( _xklDpy, False );
}

Bool XklIsSameApp( Window win1, Window win2 )
{
  Window app1, app2;
  return _XklGetAppWindow( win1, &app1 ) &&
    _XklGetAppWindow( win2, &app2 ) && app1 == app2;
}

Bool XklGetState( Window win, XklState * state_return )
{
  Window appWin;

  if( !_XklGetAppWindow( win, &appWin ) )
  {
    if( state_return != NULL )
      state_return->group = -1;
    return False;
  }

  return _XklGetAppState( appWin, state_return );
}

void XklDelState( Window win )
{
  Window appWin;

  if( _XklGetAppWindow( win, &appWin ) )
    _XklDelAppState( appWin );
}

void XklSaveState( Window win, XklState * state )
{
  Window appWin;

  if( _XklGetAppWindow( win, &appWin ) )
    _XklSaveAppState( appWin, state );
}

/**
 * Updates current internal state from X state
 */
void _XklGetRealState( XklState * curState_return )
{
  XkbStateRec state;

  curState_return->group = 0;
  if( Success == XkbGetState( _xklDpy, XkbUseCoreKbd, &state ) )
    curState_return->group = state.locked_group;

  if( Success ==
      XkbGetIndicatorState( _xklDpy, XkbUseCoreKbd,
                            &curState_return->indicators ) )
    curState_return->indicators &= _xklXkb->indicators->phys_indicators;
  else
    curState_return->indicators = 0;

}

/**
 *  Prepares the name of window suitable for debugging (32characters long).
 */
char *_XklGetDebugWindowTitle( Window win )
{
  static char sname[33];
  char *name;
  strcpy( sname, "NULL" );
  if( win != ( Window ) NULL )
  {
    name = XklGetWindowTitle( win );
    if( name != NULL )
    {
      snprintf( sname, sizeof( sname ), "%.32s", name );
      XFree( name );
    }
  }
  return sname;
}

Window XklGetCurrentWindow(  )
{
  return _xklCurClient;
}

/**
 * Loads subtree. 
 * All the windows with WM_STATE are added.
 * All the windows within level 0 are listened for focus and property
 */
Bool _XklLoadSubtree( Window window, int level, XklState * initState )
{
  Window rwin = ( Window ) NULL,
    parent = ( Window ) NULL, *children = NULL, *child;
  int num = 0;
  Bool retval = True;

  _xklLastErrorCode =
    _XklStatusQueryTree( _xklDpy, window, &rwin, &parent, &children, &num );

  if( _xklLastErrorCode != Success )
  {
    return False;
  }

  child = children;
  while( num )
  {
    XklDebug( 150, "Looking at child " WINID_FORMAT " '%s'\n", *child,
              _XklGetDebugWindowTitle( *child ) );
    if( _XklHasWmState( *child ) )
    {
      XklDebug( 150, "It has WM_STATE so we'll add it\n" );
      _XklAddAppWindow( *child, window, True, initState );
    } else
    {
      XklDebug( 150, "It does not have have WM_STATE so we'll not add it\n" );

      if( level == 0 )
      {
        XklDebug( 150, "But we are at level 0 so we'll spy on it\n" );
        _XklSelectInputMerging( *child,
                                FocusChangeMask | PropertyChangeMask );
      } else
        XklDebug( 150, "And we are at level %d so we'll not spy on it\n",
                  level );

      retval = _XklLoadSubtree( *child, level + 1, initState );
    }

    child++;
    num--;
  }

  if( children != NULL )
    XFree( children );

  return retval;
}

/**
 * Checks whether given window has WM_STATE property (i.e. "App window").
 */
Bool _XklHasWmState( Window win )
{                               /* ICCCM 4.1.3.1 */
  Atom type = None;
  int format;
  unsigned long nitems;
  unsigned long after;
  unsigned char *data = NULL;   /* Helps in the case of BadWindow error */

  XGetWindowProperty( _xklDpy, win, _xklAtoms[WM_STATE], 0, 0, False,
                      _xklAtoms[WM_STATE], &type, &format, &nitems, &after,
                      &data );
  if( data != NULL )
    XFree( data );              /* To avoid an one-byte memory leak because after successfull return
                                 * data array always contains at least one nul byte (NULL-equivalent) */
  return type != None;
}

/**
 * Finds out the official parent window (accortind to XQueryTree)
 */
Window _XklGetRegisteredParent( Window win )
{
  Window parent = ( Window ) NULL, rw = ( Window ) NULL, *children = NULL;
  unsigned nchildren = 0;

  _xklLastErrorCode =
    _XklStatusQueryTree( _xklDpy, win, &rw, &parent, &children, &nchildren );

  if( children != NULL )
    XFree( children );

  return _xklLastErrorCode == Success ? parent : ( Window ) NULL;
}

/**
 * Make sure about the result. Origial XQueryTree is pretty stupid beast:)
 */
Status _XklStatusQueryTree( Display * display,
                            Window w,
                            Window * root_return,
                            Window * parent_return,
                            Window ** children_return,
                            signed int *nchildren_return )
{
  Bool result;

  result = ( Bool ) XQueryTree( display,
                                w,
                                root_return,
                                parent_return,
                                children_return, nchildren_return );
  if( !result )
  {
    XklDebug( 160,
              "Could not get tree info for window " WINID_FORMAT ": %d\n", w,
              result );
    _xklLastErrorMsg = "Could not get the tree info";
  }

  return result ? Success : FirstExtensionError;
}

/*
 * Actually taken from mxkbledpanel, valueChangedProc
 */
Bool _XklSetIndicator( int indicatorNum, Bool set )
{
  XkbIndicatorMapPtr map;

  map = _xklXkb->indicators->maps + indicatorNum;

  /* The 'flags' field tells whether this indicator is automatic
   * (XkbIM_NoExplicit - 0x80), explicit (XkbIM_NoAutomatic - 0x40),
   * or neither (both - 0xC0).
   *
   * If NoAutomatic is set, the server ignores the rest of the 
   * fields in the indicator map (i.e. it disables automatic control 
   * of the LED).   If NoExplicit is set, the server prevents clients 
   * from explicitly changing the value of the LED (using the core 
   * protocol *or* XKB).   If NoAutomatic *and* NoExplicit are set, 
   * the LED cannot be changed (unless you change the map first).   
   * If neither NoAutomatic nor NoExplicit are set, the server will 
   * change the LED according to the indicator map, but clients can 
   * override that (until the next automatic change) using the core 
   * protocol or XKB.
   */
  switch ( map->flags & ( XkbIM_NoExplicit | XkbIM_NoAutomatic ) )
  {
    case XkbIM_NoExplicit | XkbIM_NoAutomatic:
    {
      // Can do nothing. Just ignore the indicator
      return True;
    }

    case XkbIM_NoAutomatic:
    {
      if( _xklXkb->names->indicators[indicatorNum] != None )
        XkbSetNamedIndicator( _xklDpy, XkbUseCoreKbd,
                              _xklXkb->names->indicators[indicatorNum], set,
                              False, NULL );
      else
      {
        XKeyboardControl xkc;
        xkc.led = indicatorNum;
        xkc.led_mode = set ? LedModeOn : LedModeOff;
        XChangeKeyboardControl( _xklDpy, KBLed | KBLedMode, &xkc );
        XSync( _xklDpy, 0 );
      }

      return True;
    }

    case XkbIM_NoExplicit:
      break;
  }

  /* The 'ctrls' field tells what controls tell this indicator to
   * to turn on:  RepeatKeys (0x1), SlowKeys (0x2), BounceKeys (0x4),
   *              StickyKeys (0x8), MouseKeys (0x10), AccessXKeys (0x20),
   *              TimeOut (0x40), Feedback (0x80), ToggleKeys (0x100),
   *              Overlay1 (0x200), Overlay2 (0x400), GroupsWrap (0x800),
   *              InternalMods (0x1000), IgnoreLockMods (0x2000),
   *              PerKeyRepeat (0x3000), or ControlsEnabled (0x4000)
   */
  if( map->ctrls )
  {
    unsigned long which = map->ctrls;

    XkbGetControls( _xklDpy, XkbAllControlsMask, _xklXkb );
    if( set )
      _xklXkb->ctrls->enabled_ctrls |= which;
    else
      _xklXkb->ctrls->enabled_ctrls &= ~which;
    XkbSetControls( _xklDpy, which | XkbControlsEnabledMask, _xklXkb );
  }

  /* The 'which_groups' field tells when this indicator turns on
   *      * for the 'groups' field:  base (0x1), latched (0x2), locked (0x4),
   *           *                          or effective (0x8).
   *                */
  if( map->groups )
  {
    int i;
    unsigned int group = 1;

    /* Turning on a group indicator is kind of tricky.  For
     * now, we will just Latch or Lock the first group we find
     * if that is what this indicator does.  Otherwise, we're
     * just going to punt and get out of here.
     */
    if( set )
    {
      for( i = XkbNumKbdGroups; --i >= 0; )
        if( ( 1 << i ) & map->groups )
        {
          group = i;
          break;
        }
      if( map->which_groups & ( XkbIM_UseLocked | XkbIM_UseEffective ) )
      {
        // Important: Groups should be ignored here - because they are handled separately!
        // XklLockGroup( group );
      } else if( map->which_groups & XkbIM_UseLatched )
        XkbLatchGroup( _xklDpy, XkbUseCoreKbd, group );
      else
      {
        // Can do nothing. Just ignore the indicator
        return True;
      }
    } else
      /* Turning off a group indicator will mean that we just
       * Lock the first group that this indicator doesn't watch.
       */
    {
      for( i = XkbNumKbdGroups; --i >= 0; )
        if( !( ( 1 << i ) & map->groups ) )
        {
          group = i;
          break;
        }
      XklLockGroup( group );
    }
  }

  /* The 'which_mods' field tells when this indicator turns on
   * for the modifiers:  base (0x1), latched (0x2), locked (0x4),
   *                     or effective (0x8).
   *
   * The 'real_mods' field tells whether this turns on when one of 
   * the real X modifiers is set:  Shift (0x1), Lock (0x2), Control (0x4),
   * Mod1 (0x8), Mod2 (0x10), Mod3 (0x20), Mod4 (0x40), or Mod5 (0x80). 
   *
   * The 'virtual_mods' field tells whether this turns on when one of
   * the virtual modifiers is set.
   *
   * The 'mask' field tells what real X modifiers the virtual_modifiers
   * map to?
   */
  if( map->mods.real_mods || map->mods.mask )
  {
    unsigned int affect, mods;

    affect = ( map->mods.real_mods | map->mods.mask );

    mods = set ? affect : 0;

    if( map->which_mods & ( XkbIM_UseLocked | XkbIM_UseEffective ) )
      XkbLockModifiers( _xklDpy, XkbUseCoreKbd, affect, mods );
    else if( map->which_mods & XkbIM_UseLatched )
      XkbLatchModifiers( _xklDpy, XkbUseCoreKbd, affect, mods );
    else
    {
      return True;
    }
  }

  return True;
}

const char *_XklGetEventName( int type )
{
  // Not really good to use the fact of consecutivity
  // but X protocol is already standartized so...
  static const char *evtNames[] = {
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify", "ClientMessage", "MappingNotify", "LASTEvent"
  };
  type -= KeyPress;
  if( type < 0 || type > ( sizeof( evtNames ) / sizeof( evtNames[0] ) ) )
    return NULL;
  return evtNames[type];
}
