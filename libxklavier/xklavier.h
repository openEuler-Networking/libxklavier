/**
 * @file xklavier.h
 */

#ifndef __XKLAVIER_H__
#define __XKLAVIER_H__

#include <stdarg.h>

#include <X11/Xlib.h>

#include <glib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
/**
 * Group was changed
 */
    GROUP_CHANGED,
/**
 * Indicators were changed
 */
    INDICATORS_CHANGED
  }
  XklStateChange;

/**
 * Backend allows to toggls indicators on/off
 */  
#define XKLF_CAN_TOGGLE_INDICATORS 0x01

/**
 * Backend allows to write ascii representation of the configuration
 */
#define XKLF_CAN_OUTPUT_CONFIG_AS_ASCII 0x02
  
/**
 * Backend allows to write binary representation of the configuration
 */
#define XKLF_CAN_OUTPUT_CONFIG_AS_BINARY 0x04

/**
 * Backend supports multiple layouts
 */
#define XKLF_MULTIPLE_LAYOUTS_SUPPORTED 0x08

/**
 * Backend requires manual configuration, 
 * some daemon should do 
 * xkl_StartListen( XKLL_MANAGE_LAYOUTS );
 */
#define XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT 0x10

/**
 * XKB state. Can be global or per-window
 */
  typedef struct
  {
/** selected group */
    gint32 group;
/** set of active indicators */
    guint32 indicators;
  }
  XklState;

/**
 * @defgroup xklinitterm Library initialization and termination
 * @{
 */

/**
 * Initializes internal structures. Does not start actual listening though.
 * Some apps can use xkl_avier for information retrieval but not for actual
 * processing.
 * @param dpy is an open display, will be tested for XKB extension
 * @return 0 if OK, otherwise last X error 
 * (special case: -1 if XKB extension is not present)
 */
  extern gint xkl_init( Display * dpy );

/**
 * Terminates everything...
 */
  extern gint xkl_term( void );

/**
 * What kind of backend if used
 * @return some string id of the backend
 */
  extern const gchar *xkl_backend_get_name( void );

/**
 * Provides information regarding available backend features
 * (combination of XKLF_* constants)
 * @return ORed XKLF_* constants
 */
  extern guint xkl_backend_get_features( void );

/**
 * Provides the information on maximum number of simultaneously supported 
 * groups (layouts)
 * @return maximum number of the groups in configuration, 
 *         0 if no restrictions.
 */
  extern unsigned xkl_groups_get_max_num( void );
/** @} */

/**
 * @defgroup xkbevents XKB event handling and management
 * @{
 */

/**
 * The listener process should handle the per-window states 
 * and all the related activity
 */
#define XKLL_MANAGE_WINDOW_STATES   0x01

/**
 * Just track the state and pass it to the application above.
 */
#define XKLL_TRACK_KEYBOARD_STATE 0x02

/**
 * The listener process should help backend to maintain the configuration
 * (manually switch layouts etc).
 */
#define XKLL_MANAGE_LAYOUTS  0x04

/**
 * Starts listening for XKB-related events
 * @param what any combination of XKLL_* constants
 * @return 0
 */
  extern gint xkl_listen_start( guint what );

/**
 * Stops listening for XKB-related events
 * @return 0
 */
  extern gint xkl_listen_stop( void );

/**
 * Temporary pauses listening for XKB-related events
 * @return 0
 */
  extern gint xkl_listen_pause( void );

/**
 * Resumes listening for XKB-related events
 * @return 0
 */
  extern gint xkl_listen_resume( void );

/**
 * Grabs some key
 * @param keycode is a keycode
 * @param modifiers is a bitmask of modifiers
 * @return True on success
 */
  extern gboolean xkl_key_grab( gint keycode, unsigned modifiers );

/**
 * Ungrabs some key
 * @param keycode is a keycode
 * @param modifiers is a bitmask of modifiers
 * @return True on success
 */
  extern gboolean xkl_key_ungrab( gint keycode, unsigned modifiers );

/**
 * Processes X events. Should be included into the main event cycle of an
 * application. One of the most important functions. 
 * @param evt is delivered X event
 * @return 0 if the event it processed - 1 otherwise
 * @see xkl_StartListen
 */
  extern gint xkl_events_filter( XEvent * evt );

/**
 * Allows to switch (once) to the secondary group
 */
  extern void xkl_group_allow_one_switch_to_secondary( void );

/** @} */

/**
 * @defgroup currents Current state of the library
 * @{
 */

/**
 * @return currently focused window
 */
  extern Window xkl_window_get_current( void );

/**
 * @return current state of the keyboard (in XKB terms). 
 * Returned value is a statically allocated buffer, should not be freed.
 */
  extern XklState *xkl_state_get_current( void );

/** @} */

/**
 * @defgroup wininfo Per-window information
 * @{
 */

/**
 * @return the window title of some window or NULL. 
 * If not NULL, it should be freed with XFree
 */
  extern gchar *xkl_window_get_title( Window w );

/** 
 * Finds the state for a given window (for its "App window").
 * @param win is a target window
 * @param state_out is a structure to store the state
 * @return True on success, otherwise False 
 * (the error message can be obtained using xkl_GetLastError).
 */
  extern gboolean xkl_state_get( Window win, XklState * state_out );

/**
 * Drops the state of a given window (of its "App window").
 * @param win is a target window
 */
  extern void xkl_state_delete( Window win );

/** 
 * Stores ths state for a given window
 * @param win is a target window
 * @param state is a new state of the window
 */
  extern void xkl_state_save( Window win, XklState * state );

/**
 * Sets the "transparent" flag. It means focus switching onto 
 * this window will never change the state.
 * @param win is the window do set the flag for.
 * @param transparent - if true, the windows is transparent.
 * @see xkl_IsTranspatent
 */
  extern void xkl_window_set_transparent( Window win, gboolean transparent );

/**
 * Returns "transparent" flag. 
 * @param win is the window to get the transparent flag from.
 * @see xkl_SetTranspatent
 */
  extern gboolean xkl_window_is_transparent( Window win );

/**
 * Checks whether 2 windows have the same topmost window
 * @param win1 is first window
 * @param win2 is second window
 * @return True is windows are in the same application
 */
  extern gboolean xkl_windows_is_same_appication( Window win1, Window win2 );

/** @} */

/**
 * @defgroup xkbinfo Various XKB configuration info
 * @{
 */

/**
 * @return the total number of groups in the current XKB configuration 
 * (keyboard)
 */
  extern unsigned xkl_groups_get_num( void );

/**
 * @return the array of group names for the current XKB configuration 
 * (keyboard).
 * This array is static, should not be freed
 */
  extern const gchar **xkl_groups_get_names( void );

/**
 * @return the array of indicator names for the current XKB configuration 
 * (keyboard).
 * This array is static, should not be freed
 */
  extern const gchar **xkl_indicators_get_names( void );

/** @} */

/**
 * @defgroup xkbgroup XKB group calculation and change
 * @{
 */

/**
 * Calculates next group id. Does not change the state of anything.
 * @return next group id
 */
  extern gint xkl_group_get_next( void );

/**
 * Calculates prev group id. Does not change the state of anything.
 * @return prev group id
 */
  extern gint xkl_group_get_prev( void );

/**
 * @return saved group id of the current client. 
 * Does not change the state of anything.
 */
  extern gint xkl_group_get_restore( void );

/**
 * Locks the group. Can be used after xkl_GetXXXGroup functions
 * @param group is a group number for locking
 * @see xkl_GetNextGroup
 * @see xkl_GetPrevGroup
 * @see xkl_GetRestoreGroup
 */
  extern void xkl_group_lock( gint group );

/** @} */

/**
 * @defgroup callbacks Application callbacks support
 * @{
 */

/**
 * Used for notifying application of the XKB configuration change.
 * @param data is anything which can be stored into the pointer
 * @see xkl_RegisterConfigCallback
 */
  typedef void ( *XklConfigNotifyFunc ) ( gpointer data );

/**
 * Registers user callback. Only one callback can be registered at a time
 * @param func is the function to call
 * @param data is the data to pass
 * @see xkl_ConfigCallback
 */
  extern gint xkl_register_config_callback( XklConfigNotifyFunc func,
                                            gpointer data );

/**
 * Used for notifying application of new window creation (actually, 
 * registration).
 * @param win is a new window
 * @param parent is a new window's parent
 * @param data is anything which can be stored into the pointer
 * @return the initial group id for the window (-1 to use the default value)
 * @see xkl_register_config_callback
 * @see xkl_set_default_group
 * @see xkl_get_default_group
 */
  typedef gint ( *XklNewWindowNotifyFunc ) ( Window win, Window parent,
                                             gpointer data );

/**
 * Registers user callback. Only one callback can be registered at a time
 * @param func is the function to call
 * @param data is the data to pass
 * @see XklWindowCallback
 */
  extern gint xkl_register_new_window_callback( XklNewWindowNotifyFunc func, 
                                                gpointer data );

/**
 * Used for notifying application of the window state change.
 * @param changeType is a mask of changes
 * @param group is a new group
 * @param restore is indicator of whether this state is restored from
 * saved state of set as new.
 * @param data is anything which can be stored into the pointer
 * @see xkl_register_config_callback
 */
  typedef void ( *XklStateNotifyFunc ) ( XklStateChange changeType, gint group,
                                         gboolean restore, gpointer data );

/**
 * Registers user callback. Only one callback can be registered at a time
 * @param func is the function to call
 * @param data is the data to pass
 * @see XklStateNotifyFunc
 */
  extern gint xkl_register_state_callback( XklStateNotifyFunc func,
                                           gpointer data );

/** @} */

/**
 * @defgroup settings Settings for event processing
 * @{
 */

/**
 * Sets the configuration parameter: group per application
 * @param isGlobal is a new parameter value
 */
  extern void xkl_set_group_per_toplevel_window( gboolean isGlobal );

/**
 *  @return the value of the parameter: group per application
 */
  extern gboolean xkl_is_group_per_toplevel_window( void );

/**
 * Sets the configuration parameter: perform indicators handling
 * @param whetherHandle is a new parameter value
 */
  extern void xkl_set_indicators_handling( gboolean whetherHandle );

/**
 * @return the value of the parameter: perform indicator handling
 */
  extern gboolean xkl_get_indicators_handling( void );

/**
 * Sets the secondary groups (one bit per group). 
 * Secondary groups require explicit "allowance" for switching
 * @param mask is a new group mask
 * @see xkl_allow_one_switch_to_secondary_group
 */
  extern void xkl_set_secondary_groups_mask( guint mask );

/**
 * @return the secondary group mask
 */
  extern guint xkl_get_secondary_groups_mask( void );

/**
 * Configures the default group set on window creation.
 * If -1, no default group is used
 * @param group the default group
 */
  extern void xkl_group_set_default( gint group );

/**
 * Returns the default group set on window creation
 * If -1, no default group is used
 * @return the default group
 */
  extern gint xkl_group_get_default( void );

/** @} */

/**
 * @defgroup debugerr Debugging, error processing 
 * @{
 */

/**
 * @return the text message (statically allocated) of the last error
 */
  extern const gchar *xkl_get_last_error( void );

/**
 * Output (optionally) some debug info
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @see xkl_debug
 */
  extern void _xkl_debug( const gchar file[], const gchar function[], 
                          gint level, const gchar format[], ... );

/**
 * Custom log output method for _xkl_debug. This appender is NOT called if the
 * level of the message is greater than currently set debug level.
 *
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @param args is the list of parameters
 * @see _xkl_debug
 * @see xkl_set_debug_level
 */
  typedef void ( *XklLogAppender ) ( const gchar file[], 
                                     const gchar function[],
                                     gint level, 
                                     const gchar format[],
                                     va_list args );

/**
 * Default log output method. Sends everything to stdout.
 *
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @param args is the list of parameters
 */
  extern void xkl_default_log_appender( const gchar file[], 
                                        const gchar function[],
                                        gint level, 
                                        const gchar format[],
                                        va_list args );

/**
 * Installs the custom log appender.function
 * @param fun is the new log appender
 */
  extern void xkl_set_log_appender( XklLogAppender fun );

/**
 * Sets maximum debug level. 
 * Message of the level more than the one set here - will be ignored
 * @param level is a new debug level
 */
  extern void xkl_set_debug_level( gint level );

/* Just to make doxygen happy - two block with/without @param format */
#if defined(G_HAVE_GNUC_VARARGS)
/**
 * Output (optionally) some debug info
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @see _xkl_Debug
 */
#else
/**
 * Output (optionally) some debug info
 * @param level is a level of the message
 * @see _xkl_Debug
 */
#endif
#ifdef G_HAVE_ISO_VARARGS
#define xkl_debug( level, ... ) \
  _xkl_debug( __FILE__, __func__, level, __VA_ARGS__ )
#elif defined(G_HAVE_GNUC_VARARGS)
#define xkl_debug( level, format, args... ) \
   _xkl_debug( __FILE__, __func__, level, format, ## args )
#else
#define xkl_debug( level, ... ) \
  _xkl_debug( __FILE__, __func__, level, __VA_ARGS__ )
#endif

/** @} */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif
