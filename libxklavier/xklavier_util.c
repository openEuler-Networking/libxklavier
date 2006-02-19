#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

XklState *xkl_state_get_current(  )
{
  return &xkl_current_state;
}

const gchar *xkl_get_last_error(  )
{
  return xkl_last_error_message;
}

gchar *xkl_window_get_title( Window w )
{
  Atom type_ret;
  int format_ret;
  unsigned long nitems, rest;
  unsigned char *prop;

  if( Success == XGetWindowProperty( xkl_display, w, xkl_atoms[WM_NAME], 0L,
                                     -1L, False, XA_STRING, &type_ret,
                                     &format_ret, &nitems, &rest, &prop ) )
    return (gchar *)prop;
  else
    return NULL;
}

gboolean xkl_windows_is_same_application( Window win1, Window win2 )
{
  Window app1, app2;
  return xkl_toplevel_window_find( win1, &app1 ) &&
         xkl_toplevel_window_find( win2, &app2 ) && app1 == app2;
}

gboolean xkl_state_get( Window win, XklState * state_out )
{
  Window app_win;

  if( !xkl_toplevel_window_find( win, &app_win ) )
  {
    if( state_out != NULL )
      state_out->group = -1;
    return FALSE;
  }

  return xkl_toplevel_window_get_state( app_win, state_out );
}

void xkl_state_delete( Window win )
{
  Window app_win;

  if( xkl_toplevel_window_find( win, &app_win ) )
    xkl_toplevel_window_remove_state( app_win );
}

void xkl_state_save( Window win, XklState * state )
{
  Window app_win;

  if( !( xkl_listener_type & XKLL_MANAGE_WINDOW_STATES ) )
    return;

  if( xkl_toplevel_window_find( win, &app_win ) )
    xkl_toplevel_window_save_state( app_win, state );
}

/**
 *  Prepares the name of window suitable for debugging (32characters long).
 */
gchar *xkl_window_get_debug_title( Window win )
{
  static gchar sname[33];
  gchar *name;
  strcpy( sname, "NULL" );
  if( win != ( Window ) NULL )
  {
    name = xkl_window_get_title( win );
    if( name != NULL )
    {
      snprintf( sname, sizeof( sname ), "%.32s", name );
      g_free( name );
    }
  }
  return sname;
}

Window xkl_window_get_current(  )
{
  return xkl_current_client;
}

/**
 * Loads subtree. 
 * All the windows with WM_STATE are added.
 * All the windows within level 0 are listened for focus and property
 */
gboolean xkl_load_subtree( Window window, gint level, XklState * init_state )
{
  Window rwin = ( Window ) NULL,
    parent = ( Window ) NULL, *children = NULL, *child;
  guint num = 0;
  gboolean retval = True;

  xkl_last_error_code =
    xkl_status_query_tree( window, &rwin, &parent, &children, &num );

  if( xkl_last_error_code != Success )
  {
    return FALSE;
  }

  child = children;
  while( num )
  {
    if( xkl_window_has_wm_state( *child ) )
    {
      xkl_debug( 160, "Window " WINID_FORMAT " '%s' has WM_STATE so we'll add it\n",
                 *child, xkl_window_get_debug_title( *child )  );
      xkl_toplevel_window_add( *child, window, TRUE, init_state );
    } else
    {
      xkl_debug( 200, "Window " WINID_FORMAT " '%s' does not have have WM_STATE so we'll not add it\n",
                 *child, xkl_window_get_debug_title( *child ) );

      if( level == 0 )
      {
        xkl_debug( 200, "But we are at level 0 so we'll spy on it\n" );
        xkl_select_input_merging( *child,
                                  FocusChangeMask | PropertyChangeMask );
      } else
        xkl_debug( 200, "And we are at level %d so we'll not spy on it\n",
                   level );

      retval = xkl_load_subtree( *child, level + 1, init_state );
    }

    child++;
    num--;
  }

  if( children != NULL )
    XFree( children );

  return retval;
}

/**
 * Checks whether given window has WM_STATE property (i.e. "App window").
 */
gboolean xkl_window_has_wm_state( Window win )
{                               /* ICCCM 4.1.3.1 */
  Atom type = None;
  int format;
  unsigned long nitems;
  unsigned long after;
  unsigned char *data = NULL;   /* Helps in the case of BadWindow error */

  XGetWindowProperty( xkl_display, win, xkl_atoms[WM_STATE], 0, 0, False,
                      xkl_atoms[WM_STATE], &type, &format, &nitems, &after,
                      &data );
  if( data != NULL )
    XFree( data );              /* To avoid an one-byte memory leak because after successfull return
                                 * data array always contains at least one nul byte (NULL-equivalent) */
  return type != None;
}

/**
 * Finds out the official parent window (accortind to XQueryTree)
 */
Window xkl_get_registered_parent( Window win )
{
  Window parent = ( Window ) NULL, rw = ( Window ) NULL, *children = NULL;
  guint nchildren = 0;

  xkl_last_error_code =
    xkl_status_query_tree( win, &rw, &parent, &children, &nchildren );

  if( children != NULL )
    XFree( children );

  return xkl_last_error_code == Success ? parent : ( Window ) NULL;
}

/**
 * Make sure about the result. Origial XQueryTree is pretty stupid beast:)
 */
Status xkl_status_query_tree( Window w,
                              Window * root_out,
                              Window * parent_out,
                              Window ** children_out,
                              guint *nchildren_out )
{
  gboolean result;
  unsigned int nc;

  result = ( gboolean ) XQueryTree( xkl_display,
                                w,
                                root_out,
                                parent_out,
                                children_out, &nc );
  *nchildren_out = nc;

  if( !result )
  {
    xkl_debug( 160,
               "Could not get tree info for window " WINID_FORMAT ": %d\n", w,
               result );
    xkl_last_error_message = "Could not get the tree info";
  }

  return result ? Success : FirstExtensionError;
}

const gchar *xkl_event_get_name( gint type )
{
  /* Not really good to use the fact of consecutivity
     but X protocol is already standartized so... */
  static const gchar *evt_names[] = {
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify", "ClientMessage", "MappingNotify", "LASTEvent"
  };
  type -= KeyPress;
  if( type < 0 || 
      type >= ( sizeof( evt_names ) / sizeof( evt_names[0] ) ) )
    return "UNKNOWN";
  return evt_names[type];
}

void xkl_current_state_update( int group, unsigned indicators, const char reason[] )
{
  xkl_debug( 150, 
             "Updating the current state with [g:%d/i:%u], reason: %s\n", 
             group, indicators, reason );
  xkl_current_state.group = group;
  xkl_current_state.indicators = indicators;
}
