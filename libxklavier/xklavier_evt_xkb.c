#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

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

  if( !( _xklListenerType &
         ( XKLL_MANAGE_WINDOW_STATES | XKLL_TRACK_KEYBOARD_STATE ) ) )
    return 0;

  XklDebug( 150, "Xkb event detected\n" );
  
  switch ( kev->any.xkb_type )
  {
    /**
     * Group is changed!
     */
    case XkbStateNotify:
#define GROUP_CHANGE_MASK \
    ( XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask )

      XklDebug( 150,
                "XkbStateNotify detected, changes: %X/(mask %X), new group %d\n",
                kev->state.changed, GROUP_CHANGE_MASK,
                kev->state.locked_group );

      if( kev->state.changed & GROUP_CHANGE_MASK )
        _XklStateModificationHandler( GROUP_CHANGED, 
                                      kev->state.locked_group, 
                                      0, 
                                      False );
      else /* ...not interested... */
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

    /**
     * Indicators are changed!
     */
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

      _XklStateModificationHandler( INDICATORS_CHANGED, 
                                    0, 
                                    inds, 
                                    True );
      break;

    /**
     * The configuration is changed!
     */
    case XkbIndicatorMapNotify:
    case XkbControlsNotify:
    case XkbNamesNotify:
      /* not really fair - but still better than flooding... */
      XklDebug( 200, "warning: configuration event %s is not actually processed\n",
                _XklXkbGetXkbEventName( kev->any.xkb_type ) );
      break;
    case XkbNewKeyboardNotify:
      XklDebug( 150, "%s\n",
                _XklXkbGetXkbEventName( kev->any.xkb_type ) );
      _XklResetAllInfo( "XKB event: XkbNewKeyboardNotify" );
      break;

    /**
     * ...Not interested...
     */
    default:
      XklDebug( 150, "Unknown XKB event %d [%s]\n", 
                kev->any.xkb_type, _XklXkbGetXkbEventName( kev->any.xkb_type ) );
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
