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
  struct stat statBuf;
  char fileName[MAXPATHLEN] = "";
  char* rf = _XklGetRulesSetName( "" );

  if ( rf == NULL || rf[0] == '\0' )
    return False;

  snprintf( fileName, sizeof fileName, XMODMAP_BASE "/%s.xml", rf );

  if( stat( fileName, &statBuf ) != 0 )
  {
    _xklLastErrorMsg = "No rules file found";    
    return False;
  }

  return XklConfigLoadRegistryFromFile( fileName );
}

Bool _XklXmmConfigActivate( const XklConfigRecPtr data )
{
  Bool rv;
  rv = XklSetNamesProp( xklVTable->baseConfigAtom, 
                        currentXmmRules, 
                        data );
  if( rv )
    _XklXmmLockGroup( 0 );
  return rv;
}
