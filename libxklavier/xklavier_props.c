#include <errno.h>
#include <string.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier.h"
#include "xklavier_config.h"
#include "xklavier_private.h"

void XklConfigRecInit( XklConfigRecPtr data )
{
  // clear the structure VarDefsPtr...
  memset( ( void * ) data, 0, sizeof( XklConfigRec ) );
}

void XklConfigRecDestroy( XklConfigRecPtr data )
{
  int i;
  char **p;

  if( data->model != NULL )
    free( data->model );

  if( ( p = data->layouts ) != NULL )
  {
    for( i = data->numLayouts; --i >= 0; )
      free( *p++ );
    free( data->layouts );
  }

  if( ( p = data->variants ) != NULL )
  {
    for( i = data->numVariants; --i >= 0; )
      free( *p++ );
    free( data->variants );
  }

  if( ( p = data->options ) != NULL )
  {
    for( i = data->numOptions; --i >= 0; )
      free( *p++ );
    free( data->options );
  }
}

void XklConfigRecReset( XklConfigRecPtr data )
{
  XklConfigRecDestroy( data );
  XklConfigRecInit( data );
}

Bool XklConfigGetFromServer( XklConfigRecPtr data )
{
  char *rulesFile = NULL;
  Bool rv =
    XklGetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], &rulesFile, data );
  if( rulesFile != NULL )
    free( rulesFile );

  return rv;
}

Bool XklConfigGetFromBackup( XklConfigRecPtr data )
{
  char *rulesFile = NULL;
  Bool rv =
    XklGetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM_BACKUP], &rulesFile,
                     data );
  if( rulesFile != NULL )
    free( rulesFile );

  return rv;
}

Bool XklBackupNamesProp(  )
{
  char *rf;
  XklConfigRec data;
  Bool rv = True;

  XklConfigRecInit( &data );
  if( XklGetNamesProp
      ( _xklAtoms[XKB_RF_NAMES_PROP_ATOM_BACKUP], &rf, &data ) )
  {
    XklConfigRecDestroy( &data );
    if( rf != NULL )
      free( rf );
    return True;
  }
  // "backup" property is not defined
  XklConfigRecReset( &data );
  if( XklGetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], &rf, &data ) )
  {
#if 0
    int i;
    XklDebug( 150, "Original model: [%s]\n", data.model );

    XklDebug( 150, "Original layouts(%d):\n", data.numLayouts );
    for( i = data.numLayouts; --i >= 0; )
      XklDebug( 150, "%d: [%s]\n", i, data.layouts[i] );

    XklDebug( 150, "Original variants(%d):\n", data.numVariants );
    for( i = data.numVariants; --i >= 0; )
      XklDebug( 150, "%d: [%s]\n", i, data.variants[i] );

    XklDebug( 150, "Original options(%d):\n", data.numOptions );
    for( i = data.numOptions; --i >= 0; )
      XklDebug( 150, "%d: [%s]\n", i, data.options[i] );
#endif
    if( !XklSetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM_BACKUP], rf, &data ) )
    {
      XklDebug( 150, "Could not backup the configuration" );
      rv = False;
    }
    if( rf != NULL )
      free( rf );
  } else
  {
    XklDebug( 150, "Could not get the configuration for backup" );
    rv = False;
  }
  XklConfigRecDestroy( &data );

  return rv;
}

Bool XklRestoreNamesProp(  )
{
  char *rf;
  XklConfigRec data;
  Bool rv = True;

  XklConfigRecInit( &data );
  if( !XklGetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM_BACKUP], &rf, &data ) )
  {
    XklConfigRecDestroy( &data );
    return False;
  }

  if( rf != NULL )
    free( rf );

  if( !XklSetNamesProp( _xklAtoms[XKB_RF_NAMES_PROP_ATOM], rf, &data ) )
  {
    XklDebug( 150, "Could not backup the configuration" );
    rv = False;
  }
  XklConfigRecDestroy( &data );

  return rv;
}
