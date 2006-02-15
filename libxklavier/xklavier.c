#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

Display *xkl_display;

XklState xkl_current_state;

Window xkl_current_client;

Status xkl_last_error_code;

const gchar *xkl_last_error_msg;

XErrorHandler xkl_default_err_handler;

Atom xkl_atoms[TOTAL_ATOMS];

Window xkl_root_window;

gint xkl_default_group;

gboolean xkl_skip_one_restore;

guint xkl_secondary_groups_mask;

gint xkl_debug_level = 0;

guint xkl_listener_type = 0;

XklVTable *xkl_vtable = NULL;

XklConfigNotifyFunc xkl_config_callback = NULL;
gpointer xkl_config_callback_data;

XklNewWindowNotifyFunc xkl_new_window_callback = NULL;
gpointer xkl_new_window_callback_data;

static XklStateNotifyFunc xkl_state_callback = NULL;
static gpointer xkl_state_callback_data;

static XklLogAppender log_appender = xkl_default_log_appender;

static gboolean group_per_toplevel_window = TRUE;

static gboolean handle_indicators = FALSE;


void xkl_set_indicators_handling( gboolean whether_handle )
{
  handle_indicators = whether_handle;
}

gboolean xkl_get_indicators_handling( void )
{
  return handle_indicators;
}

void xkl_set_debug_level( int level )
{
  xkl_debug_level = level;
}

void xkl_set_group_per_toplevel_window( gboolean is_set )
{
  group_per_toplevel_window = is_set;
}

gboolean xkl_is_group_per_toplevel_window( void )
{
  return group_per_toplevel_window;
}

static void xkl_set_switch_to_secondary_group( gboolean val )
{
  CARD32 propval = (CARD32)val;
  XChangeProperty( xkl_display, 
                   xkl_root_window, 
                   xkl_atoms[XKLAVIER_ALLOW_SECONDARY],
                   XA_INTEGER, 32, PropModeReplace,
                   (unsigned char*)&propval, 1 );
  XSync( xkl_display, False );
}

void _xkl_allow_one_switch_to_secondary_group( void )
{
  xkl_debug( 150, "Setting allow_one_switch_to_secondary_group flag\n" );
  xkl_set_switch_to_secondary_group( TRUE );
}

gboolean xkl_is_one_switch_to_secondary_group_allowed( void )
{
  gboolean rv = FALSE;
  unsigned char *propval = NULL;
  Atom actual_type;
  int actual_format;
  unsigned long bytes_remaining;
  unsigned long actual_items;
  int result;

  result = XGetWindowProperty( xkl_display, xkl_root_window, 
                               xkl_atoms[XKLAVIER_ALLOW_SECONDARY], 0L, 1L,
                               False, XA_INTEGER, &actual_type, &actual_format,
                               &actual_items, &bytes_remaining,
                               &propval );

  if( Success == result )
  {
    if( actual_format == 32 && actual_items == 1 )
    {
      rv = (gboolean)*(Bool*)propval;
    }
    XFree( propval );
  }

  return rv;
}

void xkl_one_switch_to_secondary_group_performed( void )
{
  xkl_debug( 150, "Resetting allow_one_switch_to_secondary_group flag\n" );
  xkl_set_switch_to_secondary_group( FALSE );
}

void xkl_set_default_group( int group )
{
  xkl_default_group = group;
}

gint _xkl_get_default_group( void )
{
  return xkl_default_group;
}

void xkl_set_secondary_groups_mask( guint mask )
{
  xkl_secondary_groups_mask = mask;
}

guint xkl_get_secondary_groups_mask( void )
{
  return xkl_secondary_groups_mask;
}

gint xkl_register_config_callback( XklConfigNotifyFunc func,
                                   gpointer data )
{
  xkl_config_callback = func;
  xkl_config_callback_data = data;
  return 0;
}

gint xkl_register_state_callback( XklStateNotifyFunc func,
                                  gpointer data )
{
  xkl_state_callback = func;
  xkl_state_callback_data = data;
  return 0;
}

gint xkl_register_new_window_callback( XklNewWindowNotifyFunc func,
                                       gpointer data )
{
  xkl_new_window_callback = func;
  xkl_new_window_callback_data = data;
  return 0;
}

void xkl_set_log_appender( XklLogAppender func )
{
  log_appender = func;
}

gint xkl_start_listen( guint what )
{
  xkl_listener_type = what;
  
  if( !( xkl_vtable->features & XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT ) &&
      ( what & XKLL_MANAGE_LAYOUTS ) )
    xkl_debug( 0, "The backend does not require manual layout management - "
                 "but it is provided by the application" );
  
  xkl_listen_resume(  );
  xkl_load_window_tree(  );
  XFlush( xkl_display );
  return 0;
}

gint xkl_stop_listen( void )
{
  xkl_listen_pause(  );
  return 0;
}

gint xkl_init( Display * a_dpy )
{
  int scr;
  const gchar *sdl;
  int rv;

  sdl = g_getenv( "XKL_DEBUG" );
  if( sdl != NULL )
  {
    xkl_set_debug_level( atoi( sdl ) );
  }

  if( !a_dpy )
  {
    xkl_debug( 10, "xkl_init : display is NULL ?\n");
    return -1;
  }

  xkl_default_err_handler =
    XSetErrorHandler( ( XErrorHandler ) xkl_process_error );

  xkl_display = a_dpy;
  scr = DefaultScreen( xkl_display );
  xkl_root_window = RootWindow( xkl_display, scr );

  xkl_skip_one_restore = FALSE;
  xkl_default_group = -1;
  xkl_secondary_groups_mask = 0L;
  xkl_toplevel_window_prev = 0;

  xkl_atoms[WM_NAME] = XInternAtom( xkl_display, "WM_NAME", False );
  xkl_atoms[WM_STATE] = XInternAtom( xkl_display, "WM_STATE", False );
  xkl_atoms[XKLAVIER_STATE] = XInternAtom( xkl_display, "XKLAVIER_STATE", False );
  xkl_atoms[XKLAVIER_TRANSPARENT] =
    XInternAtom( xkl_display, "XKLAVIER_TRANSPARENT", False );
  xkl_atoms[XKLAVIER_ALLOW_SECONDARY] =
    XInternAtom( xkl_display, "XKLAVIER_ALLOW_SECONDARY", False );

  xkl_one_switch_to_secondary_group_performed();

  rv = -1;
  xkl_debug( 150, "Trying all backends:\n" );
#ifdef ENABLE_XKB_SUPPORT
  xkl_debug( 150, "Trying XKB backend\n" );
  rv = xkl_xkb_init();
#endif
#ifdef ENABLE_XMM_SUPPORT
  if( rv != 0 ) 
  {
    xkl_debug( 150, "Trying XMM backend\n" );
    rv = xkl_xmm_init();
  }
#endif
  if( rv == 0 )
  {
    xkl_debug( 150, "Actual backend: %s\n",
              xkl_backend_get_name() );
  }
  else
  {
    xkl_debug( 0, "All backends failed, last result: %d\n", rv );
    xkl_display = NULL;
  }

  return ( rv == 0 ) ?
    ( xkl_load_all_info() ? 0 : xkl_last_error_code ) : -1;
}

gint xkl_term( void )
{
  XSetErrorHandler( ( XErrorHandler ) xkl_default_err_handler );
  xkl_config_callback = NULL;
  xkl_state_callback = NULL;
  xkl_new_window_callback = NULL;

  log_appender = xkl_default_log_appender;
  xkl_free_all_info(  );

  return 0;
}

gboolean xkl_grab_key( gint keycode, guint modifiers )
{
  gboolean ret_code;
  gchar *keyname;

  if( xkl_debug_level >= 100 )
  {
    keyname = XKeysymToString( XKeycodeToKeysym( xkl_display, keycode, 0 ) );
    xkl_debug( 100, "Listen to the key %d/(%s)/%d\n", 
                   keycode, keyname, modifiers );
  }

  if( 0 == keycode )
    return FALSE;

  xkl_last_error_code = Success;

  ret_code = XGrabKey( xkl_display, keycode, modifiers, xkl_root_window,
                      TRUE, GrabModeAsync, GrabModeAsync );
  XSync( xkl_display, False );

  xkl_debug( 100, "XGrabKey recode %d/error %d\n", 
             ret_code, xkl_last_error_code );

  ret_code = ( xkl_last_error_code == Success );

  if( !ret_code )
    xkl_last_error_msg = "Could not grab the key";

  return ret_code;
}

gboolean xkl_ungrab_key( gint keycode, guint modifiers )
{
  if( 0 == keycode )
    return FALSE;

  return Success == XUngrabKey( xkl_display, keycode, 0, xkl_root_window );
}

gint _xkl_get_next_group( void )
{
  return ( xkl_current_state.group + 1 ) % xkl_groups_get_num(  );
}
                                                                                          
gint xkl_get_prev_group( void )
{
  gint n = xkl_groups_get_num(  );
  return ( xkl_current_state.group + n - 1 ) % n;
}

gint xkl_get_restore_group( void )
{
  XklState state;
  if( xkl_current_client == ( Window ) NULL )
  {
    xkl_debug( 150, "cannot restore without current client\n" );
  } else if( xkl_state_get( xkl_current_client, &state ) )
  {
    return state.group;
  } else
    xkl_debug( 150,
              "Unbelievable: current client " WINID_FORMAT
              ", '%s' has no group\n", xkl_current_client,
              xkl_get_debug_window_title( xkl_current_client ) );
  return 0;
}

void xkl_set_transparent( Window win, gboolean transparent )
{
  Window toplevel_win;
  xkl_debug( 150, "setting transparent flag %d for " WINID_FORMAT "\n",
            transparent, win );

  if( !xkl_toplevel_window_find( win, &toplevel_win ) )
  {
    xkl_debug( 150, "No toplevel window!\n" );
    toplevel_win = win;
/*    return; */
  }

  xkl_toplevel_window_set_transparent( toplevel_win, transparent );
}

gboolean xkl_is_transparent( Window win )
{
  Window toplevel_win;

  if( !xkl_toplevel_window_find( win, &toplevel_win) )
    return FALSE;
  return xkl_toplevel_window_is_transparent( toplevel_win );
}

/**
 * Loads the tree recursively.
 */
gboolean xkl_load_window_tree( void )
{
  Window focused;
  int revert;
  gboolean retval = TRUE, have_toplevel_win;

  if( xkl_listener_type & XKLL_MANAGE_WINDOW_STATES )
    retval = xkl_load_subtree( xkl_root_window, 0, &xkl_current_state );

  XGetInputFocus( xkl_display, &focused, &revert );

  xkl_debug( 160, "initially focused: " WINID_FORMAT ", '%s'\n", 
            focused, xkl_get_debug_window_title( focused ) );

  have_toplevel_win = xkl_toplevel_window_find( focused, &xkl_current_client );

  if( have_toplevel_win )
  {
    gboolean have_state = xkl_toplevel_window_get_state( xkl_current_client,
                                                         &xkl_current_state );
    xkl_debug( 160,
              "initial xkl_cur_client: " WINID_FORMAT
              ", '%s' %s state %d/%X\n", xkl_current_client,
              xkl_get_debug_window_title( xkl_current_client ),
              ( have_state ? "with" : "without" ),
              ( have_state ? xkl_current_state.group : -1 ),
              ( have_state ? xkl_current_state.indicators : -1 ) );
  } else
  {
    xkl_debug( 160,
              "Could not find initial app. "
              "Probably, focus belongs to some WM service window. "
              "Will try to survive:)" );
  }

  return retval;
}

void _xkl_debug( const gchar file[], const gchar function[], gint level,
                 const gchar format[], ... )
{
  va_list lst;

  if( level > xkl_debug_level )
    return;

  va_start( lst, format );
  if( log_appender != NULL )
    ( *log_appender ) ( file, function, level, format, lst );
  va_end( lst );
}

void xkl_default_log_appender( const gchar file[], const gchar function[],
                               gint level, const gchar format[], va_list args )
{
  time_t now = time( NULL );
  fprintf( stdout, "[%08ld,%03d,%s:%s/] \t", now, level, file, function );
  vfprintf( stdout, format, args );
}

/**
 * Just selects some events from the window.
 */
void xkl_select_input( Window win, gulong mask )
{
  if( xkl_root_window == win )
    xkl_debug( 160,
              "Someone is looking for %lx on root window ***\n",
              mask );

  XSelectInput( xkl_display, win, mask );
}

void xkl_select_input_merging( Window win, gulong mask )
{
  XWindowAttributes attrs;
  gulong oldmask = 0L, newmask;
  memset( &attrs, 0, sizeof( attrs ) );
  if( XGetWindowAttributes( xkl_display, win, &attrs ) )
    oldmask = attrs.your_event_mask;

  newmask = oldmask | mask;
  if( newmask != oldmask )
    xkl_select_input( win, newmask );
}

void xkl_try_call_state_func( XklStateChange change_type,
                              XklState * old_state )
{
  gint group = xkl_current_state.group;
  gboolean restore = old_state->group == group;

  xkl_debug( 150,
             "change_type: %d, group: %d, secondary_group_mask: %X, allowsecondary: %d\n",
             change_type, group, xkl_secondary_groups_mask,
             xkl_is_one_switch_to_secondary_group_allowed() );

  if( change_type == GROUP_CHANGED )
  {
    if( !restore )
    {
      if( ( xkl_secondary_groups_mask & ( 1 << group ) ) != 0 &&
          !xkl_is_one_switch_to_secondary_group_allowed() )
      {
        xkl_debug( 150, "secondary -> go next\n" );
        group = xkl_group_get_next(  );
        xkl_group_lock( group );
        return;                 /* we do not need to revalidate */
      }
    }
    xkl_one_switch_to_secondary_group_performed();
  }
  if( xkl_state_callback != NULL )
  {

    ( *xkl_state_callback ) ( change_type, xkl_current_state.group,
                              restore, xkl_state_callback_data );
  }
}

void xkl_ensure_vtable_inited( void )
{
  char *p;
  if ( xkl_vtable == NULL )
  {
    xkl_debug( 0, "ERROR: XKL VTable is NOT initialized.\n" );
    /* force the crash! */
    p = NULL; *p = '\0';
  }
}

const gchar *xkl_get_backend_name( void )
{
  return xkl_vtable->id;
}

guint xkl_get_backend_features( void )
{
  return xkl_vtable->features;
}

void xkl_reset_all_info( const gchar reason[] )
{
  xkl_debug( 150, "Resetting all the cached info, reason: [%s]\n", reason );
  xkl_ensure_vtable_inited();
  if( !(*xkl_vtable->if_cached_info_equals_actual_func)() )
  {
    (*xkl_vtable->free_all_info_func)();
    (*xkl_vtable->load_all_info_func)();
  } else
    xkl_debug( 100, "NOT Resetting the cache: same configuration\n" );
}

/**
 * Calling through vtable
 */
const gchar **xkl_groups_get_names( void )
{
  xkl_ensure_vtable_inited();
  return (*xkl_vtable->groups_get_names_func)();
}

guint _xkl_groups_get_num( void )
{
  xkl_ensure_vtable_inited();
  return (*xkl_vtable->groups_get_num_func)();
}

void xkl_group_lock( int group )
{
  xkl_ensure_vtable_inited();
  (*xkl_vtable->group_lock_func)( group );
}

gint xkl_listen_pause( void )
{
  xkl_ensure_vtable_inited();
  return (*xkl_vtable->listen_pause_func)();
}

gint xkl_listen_resume( void )
{
  xkl_ensure_vtable_inited();
  xkl_debug( 150, "listenerType: %x\n", xkl_listener_type );
  if( (*xkl_vtable->listen_resume_func)() )
    return 1;
  
  xkl_select_input_merging( xkl_root_window,
                            SubstructureNotifyMask | PropertyChangeMask );
  xkl_ensure_vtable_inited();
  (*xkl_vtable->get_server_state_func)( &xkl_current_state );
  return 0;
}

gboolean xkl_load_all_info( void )
{
  xkl_ensure_vtable_inited();
  return (*xkl_vtable->load_all_info_func)();
}

void xkl_free_all_info( void )
{
  xkl_ensure_vtable_inited();
  (*xkl_vtable->free_all_info_func)();
}

guint xkl_groups_get_max_num( void )
{
  xkl_ensure_vtable_inited();
  return (*xkl_vtable->groups_get_max_num_func)();
}

