#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

int XklFilterEvents( XEvent * xev )
{
  XAnyEvent *pe = ( XAnyEvent * ) xev;
  XklDebug( 400, "**> Filtering event %d of type %d from window %d\n",
            pe->serial, pe->type, pe->window );
  _XklEnsureVTableInited();
  if ( !xklVTable->xklEventHandler( xev ) )
    switch ( xev->type )
    {                           /* core events */
      case FocusIn:
        _XklFocusInEvHandler( &xev->xfocus );
        break;
      case FocusOut:
        _XklFocusOutEvHandler( &xev->xfocus );
        break;
      case PropertyNotify:
        _XklPropertyEvHandler( &xev->xproperty );
        break;
      case CreateNotify:
        _XklCreateEvHandler( &xev->xcreatewindow );
        break;
      case DestroyNotify:
        XklDebug( 150, "Window " WINID_FORMAT " destroyed\n",
                  xev->xdestroywindow.window );
        break;
      case UnmapNotify:
        XklDebug( 200, "%s\n",  _XklGetEventName( xev->type ) );
        break;
      case MapNotify:
        XklDebug( 200, "%s\n",  _XklGetEventName( xev->type ) );
        break;
      case MappingNotify:
        XklDebug( 200, "%s\n",  _XklGetEventName( xev->type ) );
        _XklFreeAllInfo(  );
        _XklLoadAllInfo(  );
        break;
      case GravityNotify:
        XklDebug( 200, "%s\n",  _XklGetEventName( xev->type ) );
        break;
      case ReparentNotify:
        XklDebug( 200, "%s\n",  _XklGetEventName( xev->type ) );
        break;                  /* Ignore these events */
      default:
      {
        XklDebug( 200, "Unknown event %d [%s]\n", 
                       xev->type, _XklGetEventName( xev->type ) );
        return 1;
      }
    }
  XklDebug( 400, "Filtered event %d of type %d from window %d **>\n",
            pe->serial, pe->type, pe->window );
  return 1;
}

/**
 * FocusIn handler
 */
void _XklFocusInEvHandler( XFocusChangeEvent * fev )
{
  Window win;
  Window appWin;
  XklState selectedWindowState;

  if( !( _xklListenerType & XKLL_MANAGE_WINDOW_STATES ) )
    return;
  
  win = fev->window;

  switch ( fev->mode )
  {
    case NotifyNormal:
    case NotifyWhileGrabbed:
      break;
    default:
      XklDebug( 160,
                "Window " WINID_FORMAT
                " has got focus during special action %d\n", win, fev->mode );
      return;
  }

  XklDebug( 150, "Window " WINID_FORMAT ", '%s' has got focus\n", win,
            _XklGetDebugWindowTitle( win ) );

  if( !_XklGetAppWindow( win, &appWin ) )
  {
    return;
  }

  XklDebug( 150, "Appwin " WINID_FORMAT ", '%s' has got focus\n", appWin,
            _XklGetDebugWindowTitle( appWin ) );

  if( XklGetState( appWin, &selectedWindowState ) )
  {
    if( _xklCurClient != appWin )
    {
      Bool oldWinTransparent, newWinTransparent;
      XklState tmpState;

      oldWinTransparent = _XklIsTransparentAppWindow( _xklCurClient );
      if( oldWinTransparent )
        XklDebug( 150, "Leaving transparent window\n" );

      /**
       * Reload the current state from the current window. 
       * Do not do it for transparent window - we keep the state from 
       * the _previous_ window.
       */
      if ( !oldWinTransparent && XklGetState ( _xklCurClient, &tmpState ) )
      {
         _XklUpdateCurState( tmpState.group, 
                             tmpState.indicators,
                             "Loading current (previous) state from the current (previous) window" );
      }

      _xklCurClient = appWin;
      XklDebug( 150, "CurClient:changed to " WINID_FORMAT ", '%s'\n",
                _xklCurClient, _XklGetDebugWindowTitle( _xklCurClient ) );

      newWinTransparent = _XklIsTransparentAppWindow( appWin );
      if( newWinTransparent )
        XklDebug( 150, "Entering transparent window\n" );

      if( XklIsGroupPerApp() == !newWinTransparent )
      {
        /* We skip restoration only if we return to the same app window */
        Bool doSkip = False;
        if( _xklSkipOneRestore )
        {
          _xklSkipOneRestore = False;
          if( appWin == _xklPrevAppWindow )
            doSkip = True;
        }

        if( doSkip )
        {
          XklDebug( 150,
                    "Skipping one restore as requested - instead, saving the current group into the window state\n" );
          _XklSaveAppState( appWin, &_xklCurState );
        } else
        {
          if( _xklCurState.group != selectedWindowState.group )
          {
            XklDebug( 150,
                      "Restoring the group from %d to %d after gaining focus\n",
                      _xklCurState.group, selectedWindowState.group );
            /**
             *  For fast mouse movements - the state is probably not updated yet
             *  (because of the group change notification being late).
             *  so we'll enforce the update. But this should only happen in GPA mode
             */
            _XklUpdateCurState( selectedWindowState.group, 
                                selectedWindowState.indicators,
                                "Enforcing fast update of the current state" );
            XklLockGroup( selectedWindowState.group );
          } else
          {
            XklDebug( 150,
                      "Both old and new focused window have group %d so no point restoring it\n",
                      selectedWindowState.group );
            _xklAllowSecondaryGroupOnce = False;
          }
        }

        if( ( xklVTable->features & XKLF_CAN_TOGGLE_INDICATORS ) && 
             XklGetIndicatorsHandling(  ) )
        {
          XklDebug( 150,
                    "Restoring the indicators from %X to %X after gaining focus\n",
                    _xklCurState.indicators, selectedWindowState.indicators );
          _XklEnsureVTableInited();
          (*xklVTable->xklSetIndicatorsHandler)( &selectedWindowState );
        } else
          XklDebug( 150,
                    "Not restoring the indicators %X after gaining focus: indicator handling is not enabled\n",
                    _xklCurState.indicators );
      } else
        XklDebug( 150,
                  "Not restoring the group %d after gaining focus: global layout (xor transparent window)\n",
                  _xklCurState.group );
    } else
      XklDebug( 150, "Same app window - just do nothing\n" );
  } else
  {
    XklDebug( 150, "But it does not have xklavier_state\n" );
    if( _XklHasWmState( win ) )
    {
      XklDebug( 150, "But it does have wm_state so we'll add it\n" );
      _xklCurClient = appWin;
      XklDebug( 150, "CurClient:changed to " WINID_FORMAT ", '%s'\n",
                _xklCurClient, _XklGetDebugWindowTitle( _xklCurClient ) );
      _XklAddAppWindow( _xklCurClient, ( Window ) NULL, False,
                        &_xklCurState );
    } else
      XklDebug( 150, "And it does have wm_state either\n" );
  }
}

/** 
 * FocusOut handler
 */
void _XklFocusOutEvHandler( XFocusChangeEvent * fev )
{
  if( !( _xklListenerType & XKLL_MANAGE_WINDOW_STATES ) )
    return;
  
  if( fev->mode != NotifyNormal )
  {
    XklDebug( 200,
              "Window " WINID_FORMAT
              " has lost focus during special action %d\n", fev->window,
              fev->mode );
    return;
  }

  XklDebug( 160, "Window " WINID_FORMAT ", '%s' has lost focus\n",
            fev->window, _XklGetDebugWindowTitle( fev->window ) );

  if( XklIsTransparent( fev->window ) )
  {

    XklDebug( 150, "Leaving transparent window!\n" );
/** 
 * If we are leaving the transparent window - we skip the restore operation.
 * This is useful for secondary groups switching from the transparent control 
 * window.
 */
    _xklSkipOneRestore = True;
  } else
  {
    Window p;
    if( _XklGetAppWindow( fev->window, &p ) )
      _xklPrevAppWindow = p;
  }
}

/**
 * PropertyChange handler
 * Interested in :
 *  + for XKLL_MANAGE_WINDOW_STATES
 *    - WM_STATE property for all windows
 *    - Configuration property of the root window
 */
void _XklPropertyEvHandler( XPropertyEvent * pev )
{
  if( 400 <= _xklDebugLevel )
  {
    char *atomName = XGetAtomName( _xklDpy, pev->atom );
    if( atomName != NULL )
    {
      XklDebug( 400, "The property '%s' changed for " WINID_FORMAT "\n",
                atomName, pev->window );
      XFree( atomName );
    } else
    {
      XklDebug( 200, "Some magic property changed for " WINID_FORMAT "\n",
                pev->window );
    }
  }

  if( _xklListenerType & XKLL_MANAGE_WINDOW_STATES )
  {
    if( pev->atom == _xklAtoms[WM_STATE] )
    {
      Bool hasXklState = XklGetState( pev->window, NULL );
  
      if( pev->state == PropertyNewValue )
      {
        XklDebug( 160, "New value of WM_STATE on window " WINID_FORMAT "\n",
                  pev->window );
        if( !hasXklState )        /* Is this event the first or not? */
        {
          _XklAddAppWindow( pev->window, ( Window ) NULL, False,
                            &_xklCurState );
        }
      } else
      {                           /* ev->xproperty.state == PropertyDelete, either client or WM can remove it, ICCCM 4.1.3.1 */
        XklDebug( 160, "Something (%d) happened to WM_STATE of window 0x%x\n",
                  pev->state, pev->window );
        _XklSelectInputMerging( pev->window, PropertyChangeMask );
        if( hasXklState )
        {
          XklDelState( pev->window );
        }
      }
    } else
      if( pev->atom ==  xklVTable->baseConfigAtom
          && pev->window == _xklRootWindow )
    {
      if( pev->state == PropertyNewValue )
      {
        XklDebug( 160, "New value of *_NAMES_PROP_ATOM on root window\n" );
        /* If root window got new *_NAMES_PROP_ATOM -
         it most probably means new keyboard config is loaded by somebody */
        _XklFreeAllInfo(  );
        _XklLoadAllInfo(  );
      }
    }
  } /* XKLL_MANAGE_WINDOW_STATES */
}

/**
 * CreateNotify handler. Just interested in properties and focus events...
 */
void _XklCreateEvHandler( XCreateWindowEvent * cev )
{
  if( !( _xklListenerType & XKLL_MANAGE_WINDOW_STATES ) )
    return;

  XklDebug( 200,
            "Under-root window " WINID_FORMAT
            "/%s (%d,%d,%d x %d) is created\n", cev->window,
            _XklGetDebugWindowTitle( cev->window ), cev->x, cev->y,
            cev->width, cev->height );

  if( !cev->override_redirect )
  {
/* ICCCM 4.1.6: override-redirect is NOT private to
* client (and must not be changed - am I right?) 
* We really need only PropertyChangeMask on this window but even in the case of
* local server we can lose PropertyNotify events (the trip time for this CreateNotify
* event + SelectInput request is not zero) and we definitely will (my system DO)
* lose FocusIn/Out events after the following call of PropertyNotifyHandler.
* So I just decided to purify this extra FocusChangeMask in the FocusIn/OutHandler. */
    _XklSelectInputMerging( cev->window,
                            PropertyChangeMask | FocusChangeMask );

    if( _XklHasWmState( cev->window ) )
    {
      XklDebug( 200,
                "Just created window already has WM_STATE - so I'll add it" );
      _XklAddAppWindow( cev->window, ( Window ) NULL, False, &_xklCurState );
    }
  }
}

/**
 * Just error handler - sometimes we get BadWindow error for already gone 
 * windows, so we'll just ignore
 */
void _XklErrHandler( Display * dpy, XErrorEvent * evt )
{
  _xklLastErrorCode = evt->error_code;
  switch ( _xklLastErrorCode )
  {
    case BadWindow:
    case BadAccess:
    {
      /* in most cases this means we are late:) */
      XklDebug( 200, "ERROR: %p, " WINID_FORMAT ", %d, %d, %d\n",
                dpy,
                ( unsigned long ) evt->resourceid,
                ( int ) evt->error_code,
                ( int ) evt->request_code, ( int ) evt->minor_code );
      break;
    }
    default:
      ( *_xklDefaultErrHandler ) ( dpy, evt );
  }
}

/**
 * Some common functionality for Xkb handler
 */
void _XklStateModificationHandler( XklStateChange changeType, 
                                   int grp, 
                                   unsigned inds,
                                   Bool setInds )
{
  Window focused, focusedApp;
  XklState oldState;
  int revert;
  Bool haveOldState = True;
  Bool setGroup = changeType == GROUP_CHANGED;

  XGetInputFocus( _xklDpy, &focused, &revert );

  if( ( focused == None ) || ( focused == PointerRoot ) )
  {
    XklDebug( 160, "Something with focus: " WINID_FORMAT "\n", focused );
    return;
  }

  /** 
   * Only if we manage states - otherwise _xklCurClient does not make sense 
   */
  if( !_XklGetAppWindow( focused, &focusedApp ) &&
      _xklListenerType & XKLL_MANAGE_WINDOW_STATES )
    focusedApp = _xklCurClient; /* what else can I do */

  XklDebug( 150, "Focused window: " WINID_FORMAT ", '%s'\n", focusedApp,
            _XklGetDebugWindowTitle( focusedApp ) );
  if( _xklListenerType & XKLL_MANAGE_WINDOW_STATES )
  {
    XklDebug( 150, "CurClient: " WINID_FORMAT ", '%s'\n", _xklCurClient,
              _XklGetDebugWindowTitle( _xklCurClient ) );

    if( focusedApp != _xklCurClient )
    {
      /**
       * If not state - we got the new window
       */
      if ( !_XklGetAppState( focusedApp, &oldState ) )
      {
        _XklUpdateCurState( grp, inds, 
                            "Updating the state from new focused window" );
        if( _xklListenerType & XKLL_MANAGE_WINDOW_STATES )
          _XklAddAppWindow( focusedApp, ( Window ) NULL, False, &_xklCurState );
      }
      /**
       * There is state - just get the state from the window
       */
      else
      {
        grp = oldState.group;
        inds = oldState.indicators;
      }
      _xklCurClient = focusedApp;
      XklDebug( 160, "CurClient:changed to " WINID_FORMAT ", '%s'\n",
                _xklCurClient, _XklGetDebugWindowTitle( _xklCurClient ) );
    }
    /* If the window already has this this state - we are just restoring it!
      (see the second parameter of stateCallback */
    haveOldState = _XklGetAppState( _xklCurClient, &oldState );
  } else /* just tracking the stuff, no smart things */
  {
    XklDebug( 160, "Just updating the current state in the tracking mode\n" );
    memcpy( &oldState, &_xklCurState, sizeof( XklState ) );
  }

  if( setGroup || haveOldState )
  {
    _XklUpdateCurState( setGroup ? grp : oldState.group,
                        setInds ? inds : oldState.indicators,
                        "Restoring the state from the window" );
  }

  if( haveOldState )
    _XklTryCallStateCallback( changeType, &oldState );

  if( _xklListenerType & XKLL_MANAGE_WINDOW_STATES )
    _XklSaveAppState( _xklCurClient, &_xklCurState );
}
