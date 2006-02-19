#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

#define SHORTCUT_OPTION_PREFIX "grp:"

gchar* current_xmm_rules = NULL;

XklConfigRec current_xmm_config;

Atom xmm_state_atom;

const gchar **xkl_xmm_groups_get_names( void )
{
  return (const gchar **)current_xmm_config.layouts;
}

void xkl_xmm_shortcuts_grab( void )
{
  const XmmShortcut *shortcut;
  const XmmSwitchOption *option = xkl_xmm_shortcut_get_current();
  
  xkl_debug( 150, "Found shortcut option: %p\n", option );
  if( option == NULL )
    return;
  
  shortcut = option->shortcuts;
  while (shortcut->keysym != XK_VoidSymbol)
  {
    int keycode = XKeysymToKeycode( xkl_display, shortcut->keysym );
    xkl_xmm_grab_ignoring_indicators( keycode, 
                                      shortcut->modifiers );
    shortcut++;
  }
}

void xkl_xmm_shortcuts_ungrab( void )
{
  const XmmShortcut *shortcut;
  const XmmSwitchOption *option = xkl_xmm_shortcut_get_current();
  
  if( option == NULL )
    return;
  
  shortcut = option->shortcuts;
  while (shortcut->keysym != XK_VoidSymbol)
  {
    int keycode = XKeysymToKeycode( xkl_display, shortcut->keysym );
    xkl_xmm_ungrab_ignoring_indicators( keycode, 
                                        shortcut->modifiers );
    shortcut++;
  }
}

XmmSwitchOption *xkl_xmm_shortcut_get_current( void )
{
  const gchar* option_name = xkl_xmm_shortcut_get_current_option_name();
  XmmSwitchOption *switch_option = all_switch_options;
  xkl_debug( 150, "Configured switch option: [%s]\n", option_name );
  if( option_name == NULL )
    return NULL;
  while( switch_option->option_name != NULL )
  {
    if( !g_ascii_strcasecmp( switch_option->option_name, option_name ) )
      return switch_option;
    switch_option++;
  }
  return NULL;
}

const gchar* xkl_xmm_shortcut_get_current_option_name( void )
{
  gchar** option = current_xmm_config.options;
  do
  {
    /* starts with "grp:" */
    if( strstr( *option, SHORTCUT_OPTION_PREFIX ) != NULL )
    {
      return *option + sizeof SHORTCUT_OPTION_PREFIX - 1;
    }
  } while ( *(++option) != NULL );
  return NULL;
}

const XmmSwitchOption *xkl_xmm_switch_option_find( gint keycode, 
                                                   guint state, 
                                                   gint* current_shortcut_rv )
{
  const XmmSwitchOption *rv = xkl_xmm_shortcut_get_current();
  
  if( rv != NULL )
  {
    XmmShortcut *sc = rv->shortcuts;
    while (sc->keysym != XK_VoidSymbol)
    {
      if( ( XKeysymToKeycode( xkl_display, sc->keysym ) == keycode ) &&
          ( ( state & sc->modifiers ) == sc->modifiers ) )
      {
        return rv;
      }
      sc++;
    }
  }
  return NULL;
}

gint xkl_xmm_listen_resume( void )
{
  if( xkl_listener_type & XKLL_MANAGE_LAYOUTS )
    xkl_xmm_shortcuts_grab();
  return 0;
}

gint xkl_xmm_listen_pause( void )
{
  if( xkl_listener_type & XKLL_MANAGE_LAYOUTS )
    xkl_xmm_shortcuts_ungrab();
  return 0;
}

guint xkl_xmm_groups_get_max_num( void )
{
  return 0;
}

guint xkl_xmm_groups_get_num( void )
{
  gint rv = 0;
  gchar ** p = current_xmm_config.layouts;
  while( *p++ != NULL ) rv++;
  return rv;
}
  
void xkl_xmm_free_all_info( void )
{
  if( current_xmm_rules != NULL )
  {
    g_free( current_xmm_rules );
    current_xmm_rules = NULL;
  }
  xkl_config_rec_reset( &current_xmm_config );
}

gboolean xkl_xmm_if_cached_info_equals_actual( void )
{
  return FALSE;
}

gboolean xkl_xmm_load_all_info(  )
{
  return xkl_config_get_full_from_server( &current_xmm_rules,
                                          &current_xmm_config );
}

void xkl_xmm_state_get_server( XklState * state )
{
  unsigned char *propval = NULL;
  Atom actual_type;
  int actual_format;
  unsigned long bytes_remaining;
  unsigned long actual_items;
  int result;
  
  memset( state, 0, sizeof( *state ) );

  result = XGetWindowProperty( xkl_display, xkl_root_window,
                               xmm_state_atom, 0L, 1L, 
                               False, XA_INTEGER, &actual_type,
                               &actual_format, &actual_items,
                               &bytes_remaining, &propval );
  
  if( Success == result )
  {
    if( actual_format == 32 || actual_items == 1 )
    {
      state->group = *(CARD32*)propval;
    } else
    {  
      xkl_debug( 160, "Could not get the xmodmap current group\n" );
    }
    XFree( propval );
  } else
  {
    xkl_debug( 160, "Could not get the xmodmap current group: %d\n", result );
  }
}

void xkl_xmm_group_actualize( gint group )
{
  char cmd[1024];
  int res;
  const gchar* layout_name = NULL;
  
  if( xkl_xmm_groups_get_num() < group )
    return;
  
  layout_name = current_xmm_config.layouts[group];
  
  snprintf( cmd, sizeof cmd, 
            "xmodmap %s/xmodmap.%s",
            XMODMAP_BASE, layout_name );

  res = system( cmd );
  if( res > 0 )
  {
    xkl_debug( 0, "xmodmap error %d\n", res );
  } else if( res < 0 )
  {
    xkl_debug( 0, "Could not execute xmodmap: %d\n", res );
  }
  XSync( xkl_display, False );
}

void xkl_xmm_group_lock( gint group )
{
  CARD32 propval;

  if( xkl_xmm_groups_get_num() < group )
    return;
  
  /* updating the status property */
  propval = group;
  XChangeProperty( xkl_display, xkl_root_window, xmm_state_atom, 
                   XA_INTEGER, 32, PropModeReplace, 
                   (unsigned char*)&propval, 1 );
  XSync( xkl_display, False );
}

gint xkl_xmm_init( void )
{
  static XklVTable xkl_xmm_vtable =
  {
    "xmodmap",
    XKLF_MULTIPLE_LAYOUTS_SUPPORTED |
      XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT, 
    xkl_xmm_config_activate,
    xkl_xmm_config_init,
    xkl_xmm_config_registry_load,
    NULL, /* no write_file */

    xkl_xmm_groups_get_names,
    xkl_xmm_groups_get_max_num,
    xkl_xmm_groups_get_num,
    xkl_xmm_group_lock,

    xkl_xmm_process_x_event,
    xkl_xmm_free_all_info,
    xkl_xmm_if_cached_info_equals_actual,
    xkl_xmm_load_all_info,
    xkl_xmm_state_get_server,
    xkl_xmm_listen_pause,
    xkl_xmm_listen_resume,
    NULL, /* no indicators_set */
  };

  if( getenv( "XKL_XMODMAP_DISABLE" ) != NULL )
    return -1;

  xkl_xmm_vtable.base_config_atom =
    XInternAtom( xkl_display, "_XMM_NAMES", False );
  xkl_xmm_vtable.backup_config_atom =
    XInternAtom( xkl_display, "_XMM_NAMES_BACKUP", False );

  xmm_state_atom =
    XInternAtom( xkl_display, "_XMM_STATE", False );
  
  xkl_xmm_vtable.default_model = "generic";
  xkl_xmm_vtable.default_layout = "us";

  xkl_vtable = &xkl_xmm_vtable;

  return 0;
}
