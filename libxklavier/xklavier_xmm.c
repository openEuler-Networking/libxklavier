#include <time.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

const char **_XklXmmGetGroupNames( void )
{
  return ( const char ** ) NULL;
}

int _XklXmmPauseListen(  )
{
  return 0;
}

int _XklXmmResumeListen(  )
{
  return 0;
}

unsigned _XklXmmGetNumGroups( void )
{
  return 0;
}

void _XklXmmFreeAllInfo(  )
{
}

Bool _XklXmmLoadAllInfo(  )
{
  return False;
}

void _XklXmmLockGroup( int group )
{
}

int _XklXmmInit( void )
{
  static XklVTable xklXmmVTable =
  {
    _XklXmmConfigActivate,
    _XklXmmConfigInit,
    _XklXmmConfigLoadRegistry,
    _XklXmmConfigMultipleLayoutsSupported,
    _XklXmmConfigWriteFile,
    _XklXmmEventHandler,
    _XklXmmFreeAllInfo,
    _XklXmmGetGroupNames,
    _XklXmmGetNumGroups,
    _XklXmmLoadAllInfo,
    _XklXmmLockGroup,
    _XklXmmPauseListen,
    _XklXmmResumeListen,
    _XklXmmSetIndicators,
  };

  xklVTable = &xklXmmVTable;

  XklDebug( 160,
            "Xmodmap support activated, display: %p, root: " WINID_FORMAT
            "\n", _xklDpy, _xklRootWindow );
  return -1;
}
