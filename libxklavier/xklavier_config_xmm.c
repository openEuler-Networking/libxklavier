#include <errno.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <fcntl.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

void _XklXmmConfigInit( void )
{
}

Bool _XklXmmConfigLoadRegistry( void )
{
  return False;
}

// check only client side support
Bool _XklXmmConfigMultipleLayoutsSupported( void )
{
  return False;
}

Bool _XklXmmConfigActivate( const XklConfigRecPtr data )
{
  return False;
}

Bool _XklXmmConfigWriteFile( const char *fileName, 
                             const XklConfigRecPtr data,
                             const Bool binary )
{
  return False;
}
