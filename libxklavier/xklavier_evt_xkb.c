#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

#ifdef XKB_HEADERS_PRESENT
/**
 * Some common functionality for Xkb handler
 */
static void _XklStdXkbHandler( int grp, XklStateChange changeType, unsigned inds,
                               Bool setInds )
{
  Window focused, focusedApp;
  XklState oldState;
  int revert;
  Bool haveState;
  Bool setGroup = changeType == GROUP_CHANGED;

  XGetInputFocus( _xklDpy, &focused, &revert );

  if( ( focused == None ) || ( focused == PointerRoot ) )
  {
    XklDebug( 160, "Something with focus: " WINID_FORMAT "\n", focused );
    return;
  }

  if( !_XklGetAppWindow( focused, &focusedApp ) )
    focusedApp = _xklCurClient; //what else can I do

  XklDebug( 150, "Focused window: " WINID_FORMAT ", '%s'\n", focusedApp,
            _XklGetDebugWindowTitle( focusedApp ) );
  XklDebug( 150, "CurClient: " WINID_FORMAT ", '%s'\n", _xklCurClient,
            _XklGetDebugWindowTitle( _xklCurClient ) );

  if( focusedApp != _xklCurClient )
  {
    if ( !_XklGetAppState( focusedApp, &oldState ) )
    {
      _XklUpdateCurState( grp, inds, 
                          "Updating the state from new focused window" );
      _XklAddAppWindow( focusedApp, ( Window ) NULL, False, &_xklCurState );
    }
    else
    {
      grp = oldState.group;
      inds = oldState.indicators;
    }
    _xklCurClient = focusedApp;
    XklDebug( 160, "CurClient:changed to " WINID_FORMAT ", '%s'\n",
              _xklCurClient, _XklGetDebugWindowTitle( _xklCurClient ) );
  }
  // if the window already has this this state - we are just restoring it!
  // (see the second parameter of stateCallback
  haveState = _XklGetAppState( _xklCurClient, &oldState );

  if( setGroup || haveState )
  {
    _XklUpdateCurState( setGroup ? grp : oldState.group,
                        setInds ? inds : oldState.indicators,
                        "Restoring the state from the window" );
  }

  if( haveState )
    _XklTryCallStateCallback( changeType, &oldState );

  _XklSaveAppState( _xklCurClient, &_xklCurState );
}
#endif

/**
 * XKB event handler
 */
int _XklXkbEventHandler( XEvent *xev )
{
#ifdef XKB_HEADERS_PRESENT
  int i;
  unsigned bit;
  unsigned inds;
  XkbEvent *kev = (XkbEvent*)xev;

  if( xev->type != _xklXkbEventType )
    return 0;

  XklDebug( 150, "Xkb event detected\n" );

  switch ( kev->any.xkb_type )
  {
    case XkbStateNotify:
#define GROUP_CHANGE_MASK \
    ( XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask )

      XklDebug( 150,
                "XkbStateNotify detected, changes: %X/(mask %X), new group %d\n",
                kev->state.changed, GROUP_CHANGE_MASK,
                kev->state.locked_group );

      if( kev->state.changed & GROUP_CHANGE_MASK )
        _XklStdXkbHandler( kev->state.locked_group, GROUP_CHANGED, 0, False );
      else
      {
        XklDebug( 200,
                  "This type of state notification is not regarding groups\n" );
        if ( kev->state.locked_group != _xklCurState.group )
          XklDebug( 0, 
                    "ATTENTION! Currently cached group %d is not equal to the current group from the event: %d\n!",
                    _xklCurState.group,
                    kev->state.locked_group );
      }

      break;

    case XkbIndicatorStateNotify:

      XklDebug( 150, "XkbIndicatorStateNotify\n" );

      inds = _xklCurState.indicators;

      ForPhysIndicators( i, bit ) if( kev->indicators.changed & bit )
      {
        if( kev->indicators.state & bit )
          inds |= bit;
        else
          inds &= ~bit;
      }

      _XklStdXkbHandler( 0, INDICATORS_CHANGED, inds, True );
      break;

    case XkbIndicatorMapNotify:
      XklDebug( 150, "XkbIndicatorMapNotify\n" );
      _XklFreeAllInfo(  );
      _XklLoadAllInfo(  );
      break;

    case XkbControlsNotify:
      XklDebug( 150, "XkbControlsNotify\n" );
      _XklFreeAllInfo(  );
      _XklLoadAllInfo(  );
      break;

    case XkbNamesNotify:
      XklDebug( 150, "XkbNamesNotify\n" );
      _XklFreeAllInfo(  );
      _XklLoadAllInfo(  );
      break;

    case XkbNewKeyboardNotify:
      XklDebug( 150, "XkbNewKeyboardNotify\n" );
      _XklFreeAllInfo(  );
      _XklLoadAllInfo(  );
      break;

    default:
      XklDebug( 150, "Unknown xkb event %d\n", kev->any.xkb_type );
      return 0;
  }
  return 1;
#else
  return 0;
#endif
}

void _XklXkbSetIndicators( const XklState *windowState )
{
#ifdef XKB_HEADERS_PRESENT
  int i;
  unsigned bit;

  ForPhysIndicators( i,
                     bit ) if( _xklXkb->names->indicators[i] != None )
  {
    Bool status;
    status = _XklSetIndicator( i,
                               ( windowState->indicators & bit ) != 0 );
    XklDebug( 150, "Set indicator \"%s\"/%d to %d: %d\n",
              _xklIndicatorNames[i],
              _xklXkb->names->indicators[i],
              windowState->indicators & bit,
              status );
  }
#endif
}
