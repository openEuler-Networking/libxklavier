#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

/**
 * XKB event handler
 */
int _XklXmmEventHandler( XEvent *xev )
{
  return 0;
}

void _XklXmmSetIndicators( const XklState *windowState )
{
}
