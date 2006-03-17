#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

static int _XklXmmKeypressEventHandler( XKeyPressedEvent* kpe )
{
  if( _xklListenerType & XKLL_MANAGE_LAYOUTS )
  {
    int currentShortcut = 0;
    XmmSwitchOptionPtr sop;
    XklDebug( 200, "Processing the KeyPress event\n" );
    sop = _XklXmmFindSwitchOption( kpe->keycode, 
                                   kpe->state,
                                   &currentShortcut );
    if( sop != NULL )
    {
      XklState state;
      XklDebug( 150, "It is THE shortcut\n" );
      _XklXmmGetRealState( &state );
      if( state.group != -1 )
      {
        int newGroup = ( state.group + sop->shortcutSteps[currentShortcut] ) % 
                         currentXmmConfig.numLayouts;
        XklDebug( 150, "Setting new xmm group %d\n", newGroup );
        _XklXmmLockGroup( newGroup );
        return 1;
      }
    }
  }
  return 0;
}

static int _XklXmmPropertyEventHandler( XPropertyEvent* kpe )
{
  XklDebug( 200, "Processing the PropertyNotify event: %d/%d\n", 
            kpe->atom, xmmStateAtom );
  /**
   * Group is changed!
   */
  if( kpe->atom == xmmStateAtom )
  {
    XklState state;
    _XklXmmGetRealState( &state );
    
    if( _xklListenerType & XKLL_MANAGE_LAYOUTS )
    {
      XklDebug( 150, "Current group from the root window property %d\n", state.group );
      _XklXmmUngrabShortcuts();
      _XklXmmActualizeGroup( state.group );
      _XklXmmGrabShortcuts();
      return 1;
    }
    
    if( _xklListenerType &
       ( XKLL_MANAGE_WINDOW_STATES | XKLL_TRACK_KEYBOARD_STATE ) )
    {
      XklDebug( 150,
                "XMM state changed, new 'group' %d\n",
                state.group );
      
      _XklStateModificationHandler( GROUP_CHANGED, 
                                    state.group, 
                                    0, 
                                    False );
    }
  } else
  /**
   * Configuration is changed!
   */
  if( kpe->atom == xklVTable->baseConfigAtom )
  {
    _XklResetAllInfo( "base config atom changed" );
  }
  
  return 0;
}

/**
 * XMM event handler
 */
int _XklXmmEventHandler( XEvent *xev )
{
  switch( xev->type )
  {
    case KeyPress:
      return _XklXmmKeypressEventHandler( (XKeyPressedEvent*)xev );
    case PropertyNotify:
      return _XklXmmPropertyEventHandler( (XPropertyEvent*)xev );
  }
  return 0;
}

/**
 * We have to find which of Shift/Lock/Control/ModX masks
 * belong to Caps/Num/Scroll lock
 */
static void _XklXmmInitXmmIndicatorsMap( int* pCapsLockMask,
                                      int* pNumLockMask,
                                      int* pScrollLockMask )
{
  XModifierKeymap *xmkm = NULL;
  KeyCode *kcmap, nlkc, clkc, slkc;
  int m, k, mask;

  xmkm = XGetModifierMapping( _xklDpy );
  if( xmkm )
  {
    clkc = XKeysymToKeycode( _xklDpy, XK_Num_Lock );
    nlkc = XKeysymToKeycode( _xklDpy, XK_Caps_Lock );
    slkc = XKeysymToKeycode( _xklDpy, XK_Scroll_Lock );

    kcmap = xmkm->modifiermap;
    mask = 1;
    for( m = 8; --m >= 0; mask <<= 1 )
      for( k = xmkm->max_keypermod; --k >= 0; kcmap++ )
      {
        if( *kcmap == clkc )
          *pCapsLockMask = mask;
        if( *kcmap == slkc )
          *pScrollLockMask = mask;
        if( *kcmap == nlkc )
          *pNumLockMask = mask;
      }
    XFreeModifiermap( xmkm );
  }
}

void _XklXmmGrabIgnoringIndicators( int keycode, int modifiers )
{
  int CapsLockMask = 0, NumLockMask = 0, ScrollLockMask = 0;
  
  _XklXmmInitXmmIndicatorsMap( &CapsLockMask, &NumLockMask, &ScrollLockMask );

#define GRAB(mods) \
  XklGrabKey( keycode, modifiers|(mods) )
  
  GRAB( 0 );
  GRAB( CapsLockMask );
  GRAB( NumLockMask );
  GRAB( ScrollLockMask );
  GRAB( CapsLockMask | NumLockMask );
  GRAB( CapsLockMask | ScrollLockMask );
  GRAB( NumLockMask | ScrollLockMask );
  GRAB( CapsLockMask | NumLockMask | ScrollLockMask );
#undef GRAB
}

void _XklXmmUngrabIgnoringIndicators( int keycode, int modifiers )
{
  int CapsLockMask = 0, NumLockMask = 0, ScrollLockMask = 0;

  _XklXmmInitXmmIndicatorsMap( &CapsLockMask, &NumLockMask, &ScrollLockMask );
  
#define UNGRAB(mods) \
  XklUngrabKey( keycode, modifiers|(mods) )
  
  UNGRAB( 0 );
  UNGRAB( CapsLockMask );
  UNGRAB( NumLockMask );
  UNGRAB( ScrollLockMask );
  UNGRAB( CapsLockMask | NumLockMask );
  UNGRAB( CapsLockMask | ScrollLockMask );
  UNGRAB( NumLockMask | ScrollLockMask );
  UNGRAB( CapsLockMask | NumLockMask | ScrollLockMask );
#undef UNGRAB
}
