#include <time.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

#ifdef XKB_HEADERS_PRESENT
XkbDescPtr _xklXkb;

static XkbDescPtr precachedXkb = NULL;

char *_xklIndicatorNames[XkbNumIndicators];

unsigned _xklPhysIndicatorsMask;

int _xklXkbEventType, _xklXkbError;

static char *groupNames[XkbNumKbdGroups];

const char **_XklXkbGetGroupNames( void )
{
  return ( const char ** ) groupNames;
}

int _XklXkbPauseListen(  )
{
  XkbSelectEvents( _xklDpy, XkbUseCoreKbd, XkbAllEventsMask, 0 );
/*  XkbSelectEventDetails( _xklDpy,
                         XkbUseCoreKbd,
                       XkbStateNotify,
                     0,
                   0 );

  !!_XklSelectInput( _xklRootWindow, 0 );
*/
  return 0;
}

int _XklXkbResumeListen(  )
{
  /* What events we want */
#define XKB_EVT_MASK \
         (XkbStateNotifyMask| \
          XkbNamesNotifyMask| \
          XkbControlsNotifyMask| \
          XkbIndicatorStateNotifyMask| \
          XkbIndicatorMapNotifyMask| \
          XkbNewKeyboardNotifyMask)

  XkbSelectEvents( _xklDpy, XkbUseCoreKbd, XKB_EVT_MASK, XKB_EVT_MASK );

#define XKB_STATE_EVT_DTL_MASK \
         (XkbGroupStateMask)

  XkbSelectEventDetails( _xklDpy,
                         XkbUseCoreKbd,
                         XkbStateNotify,
                         XKB_STATE_EVT_DTL_MASK, XKB_STATE_EVT_DTL_MASK );

#define XKB_NAMES_EVT_DTL_MASK \
         (XkbGroupNamesMask|XkbIndicatorNamesMask)

  XkbSelectEventDetails( _xklDpy,
                         XkbUseCoreKbd,
                         XkbNamesNotify,
                         XKB_NAMES_EVT_DTL_MASK, XKB_NAMES_EVT_DTL_MASK );
  return 0;
}

unsigned _XklXkbGetMaxNumGroups( void )
{
  return xklVTable->features & XKLF_MULTIPLE_LAYOUTS_SUPPORTED ?
    XkbNumKbdGroups : 1;
}

unsigned _XklXkbGetNumGroups( void )
{
  return _xklXkb->ctrls->num_groups;
}

#define KBD_MASK \
    ( 0 )
#define CTRLS_MASK \
  ( XkbSlowKeysMask )
#define NAMES_MASK \
  ( XkbGroupNamesMask | XkbIndicatorNamesMask )

void _XklXkbFreeAllInfo(  )
{
  int i;
  char **pi = _xklIndicatorNames;
  for( i = 0; i < XkbNumIndicators; i++, pi++ )
  {
    /* only free non-empty ones */
    if( *pi && **pi )
      XFree( *pi );
  }
  if( _xklXkb != NULL )
  {
    int i;
    char **groupName = groupNames;
    for( i = _xklXkb->ctrls->num_groups; --i >= 0; groupName++ )
      if( *groupName )
      {
        XFree( *groupName );
        *groupName = NULL;
      }
    XkbFreeKeyboard( _xklXkb, XkbAllComponentsMask, True );
    _xklXkb = NULL;
  }

  /* just in case - never actually happens...*/
  if( precachedXkb != NULL )
  {
    XkbFreeKeyboard( precachedXkb, XkbAllComponentsMask, True );
    precachedXkb = NULL;
  }
}

static Bool _XklXkbLoadPrecachedXkb( void )
{
  Bool rv = False;
  Status status;

  precachedXkb = XkbGetMap( _xklDpy, KBD_MASK, XkbUseCoreKbd );
  if( precachedXkb != NULL )
  {
    rv = Success == ( status = XkbGetControls( _xklDpy, CTRLS_MASK, precachedXkb ) ) &&
         Success == ( status = XkbGetNames( _xklDpy, NAMES_MASK, precachedXkb ) ) &&
         Success == ( status = XkbGetIndicatorMap( _xklDpy, XkbAllIndicatorsMask, precachedXkb ) );
    if( !rv )
    {
      _xklLastErrorMsg = "Could not load controls/names/indicators";
      XklDebug( 0, "%s: %d\n", _xklLastErrorMsg, status );
      XkbFreeKeyboard( precachedXkb, XkbAllComponentsMask, True );
    }
  }
  return rv;
}

Bool _XklXkbIfCachedInfoEqualsActual( )
{
  int i;
  Atom *pa1, *pa2;
  Bool rv = False;

  if( _XklXkbLoadPrecachedXkb() )
  {
    /* First, compare the number of groups */
    if( _xklXkb->ctrls->num_groups == precachedXkb->ctrls->num_groups )
    {
      /* Then, compare group names, just atoms */
      pa1 = _xklXkb->names->groups;
      pa2 = precachedXkb->names->groups;
      for( i = _xklXkb->ctrls->num_groups; --i >= 0; pa1++, pa2++ )
        if( *pa1 != *pa2 )
          break;

      /* Then, compare indicator names, just atoms */
      if( i < 0 )
      {
        pa1 = _xklXkb->names->indicators;
        pa2 = precachedXkb->names->indicators;
        for( i = XkbNumIndicators; --i >= 0; pa1++, pa2++ )
          if( *pa1 != *pa2 )
            break;
        rv = i < 0;
      }
    }
    /** 
     * in case of failure, reuse in _XklXkbLoadAllInfo
     * in case of success - free it
     */
    if( rv )
    {
      XkbFreeKeyboard( precachedXkb, XkbAllComponentsMask, True );
      precachedXkb = NULL;
    }
  } else
  {
    XklDebug( 0, "Could not load the XkbDescPtr for comparison\n" );
  }
  return rv;
}

/**
 * Load some XKB parameters
 */
Bool _XklXkbLoadAllInfo(  )
{
  int i;
  Atom *pa;
  char **groupName;
  char **pi = _xklIndicatorNames;

  if ( precachedXkb == NULL )
    if ( !_XklXkbLoadPrecachedXkb() )
    {
      _xklLastErrorMsg = "Could not load keyboard";
      return False;
    }

  /* take it from the cache (in most cases LoadAll is called from ResetAll which in turn ...)*/
  _xklXkb = precachedXkb;
  precachedXkb = NULL;

  /* First, output the number of the groups */
  XklDebug( 200, "found %d groups\n", _xklXkb->ctrls->num_groups );

  /* Then, cache (and output) the names of the groups */
  pa = _xklXkb->names->groups;
  groupName = groupNames;
  for( i = _xklXkb->ctrls->num_groups; --i >= 0; pa++, groupName++ )
  {
    *groupName = XGetAtomName( _xklDpy,
                               *pa == None ?
                               XInternAtom( _xklDpy, "-", False ) : *pa );
    XklDebug( 200, "group %d has name [%s]\n", i, *groupName );
  }

  _xklLastErrorCode =
    XkbGetIndicatorMap( _xklDpy, XkbAllIndicatorsMask, _xklXkb );

  if( _xklLastErrorCode != Success )
  {
    _xklLastErrorMsg = "Could not load indicator map";
    return False;
  }

  /* Then, cache (and output) the names of the indicators */
  pa = _xklXkb->names->indicators;
  for( i = XkbNumIndicators; --i >= 0; pi++, pa++ )
  {
    Atom a = *pa;
    if( a != None )
      *pi = XGetAtomName( _xklDpy, a );
    else
      *pi = "";

    XklDebug( 200, "Indicator[%d] is %s\n", i, *pi  );
  }

  XklDebug( 200, "Real indicators are %X\n",
            _xklXkb->indicators->phys_indicators );

  if( _xklConfigCallback != NULL )
    ( *_xklConfigCallback ) ( _xklConfigCallbackData );
  return True;
}

void _XklXkbLockGroup( int group )
{
  XklDebug( 100, "Posted request for change the group to %d ##\n", group );
  XkbLockGroup( _xklDpy, XkbUseCoreKbd, group );
  XSync( _xklDpy, False );
}

/**
 * Updates current internal state from X state
 */
void _XklXkbGetRealState( XklState * curState_return )
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
      /* Can do nothing. Just ignore the indicator */
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
   * for the 'groups' field:  base (0x1), latched (0x2), locked (0x4),
   * or effective (0x8).
   */
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
        /* Important: Groups should be ignored here - because they are handled separately! */
        /* XklLockGroup( group ); */
      } else if( map->which_groups & XkbIM_UseLatched )
        XkbLatchGroup( _xklDpy, XkbUseCoreKbd, group );
      else
      {
        /* Can do nothing. Just ignore the indicator */
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

#endif

int _XklXkbInit( void )
{
#ifdef XKB_HEADERS_PRESENT
  int opcode;
  static XklVTable xklXkbVTable =
  {
    "XKB",
    XKLF_CAN_TOGGLE_INDICATORS |
      XKLF_CAN_OUTPUT_CONFIG_AS_ASCII |
      XKLF_CAN_OUTPUT_CONFIG_AS_BINARY,
    _XklXkbConfigActivate,
    _XklXkbConfigInit,
    _XklXkbConfigLoadRegistry,
    _XklXkbConfigWriteFile,
    _XklXkbEventHandler,
    _XklXkbFreeAllInfo,
    _XklXkbGetGroupNames,
    _XklXkbGetMaxNumGroups,
    _XklXkbGetNumGroups,
    _XklXkbGetRealState,
    _XklXkbIfCachedInfoEqualsActual,
    _XklXkbLoadAllInfo,
    _XklXkbLockGroup,
    _XklXkbPauseListen,
    _XklXkbResumeListen,
    _XklXkbSetIndicators,
  };

  if( getenv( "XKL_XKB_DISABLE" ) != NULL )
    return -1;

  _xklXkbExtPresent = XkbQueryExtension( _xklDpy,
                                         &opcode, &_xklXkbEventType,
                                         &_xklXkbError, NULL, NULL );
  if( !_xklXkbExtPresent )
  {
    XSetErrorHandler( ( XErrorHandler ) _xklDefaultErrHandler );
    return -1;
  }
  
  XklDebug( 160,
            "xkbEvenType: %X, xkbError: %X, display: %p, root: " WINID_FORMAT
            "\n", _xklXkbEventType, _xklXkbError, _xklDpy, _xklRootWindow );

  xklXkbVTable.baseConfigAtom =
    XInternAtom( _xklDpy, _XKB_RF_NAMES_PROP_ATOM, False );
  xklXkbVTable.backupConfigAtom =
    XInternAtom( _xklDpy, "_XKB_RULES_NAMES_BACKUP", False );

  xklXkbVTable.defaultModel = "pc101";
  xklXkbVTable.defaultLayout = "us";

  xklVTable = &xklXkbVTable;

  /* First, we have to assign xklVTable - 
     because this function uses it */
  
  if( _XklXkbConfigMultipleLayoutsSupported() )
    xklXkbVTable.features |= XKLF_MULTIPLE_LAYOUTS_SUPPORTED;
  
  return 0;
#else
  XklDebug( 160,
            "NO XKB LIBS, display: %p, root: " WINID_FORMAT
            "\n", _xklDpy, _xklRootWindow );
  return -1;
#endif
}

#ifdef XKB_HEADERS_PRESENT
const char *_XklXkbGetXkbEventName( int xkb_type )
{
  /* Not really good to use the fact of consecutivity
     but XKB protocol extension is already standartized so... */
  static const char *evtNames[] = {
    "XkbNewKeyboardNotify",
    "XkbMapNotify",
    "XkbStateNotify",
    "XkbControlsNotify",
    "XkbIndicatorStateNotify",
    "XkbIndicatorMapNotify",
    "XkbNamesNotify",
    "XkbCompatMapNotify",
    "XkbBellNotify",
    "XkbActionMessage",
    "XkbAccessXNotify",
    "XkbExtensionDeviceNotify",
    "LASTEvent"
  };
  xkb_type -= XkbNewKeyboardNotify;
  if( xkb_type < 0 || 
      xkb_type >= ( sizeof( evtNames ) / sizeof( evtNames[0] ) ) )
    return "UNKNOWN";
  return evtNames[xkb_type];
}
#endif
