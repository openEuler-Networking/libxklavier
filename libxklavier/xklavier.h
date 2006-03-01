/**
 * @file xklavier.h
 */

#ifndef __XKLAVIER_H__
#define __XKLAVIER_H__

#include <stdarg.h>

#include <X11/Xlib.h>

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef struct _XklEngine XklEngine;
	typedef struct _XklEnginePrivate XklEnginePrivate;
	typedef struct _XklEngineClass XklEngineClass;

#define XKL_TYPE_ENGINE             (xkl_engine_get_type ())
#define XKL_ENGINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_ENGINE, XklEngine))
#define XKL_ENGINE_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_ENGINE,  XklEngineClass))
#define XKL_IS_ENGINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_ENGINE))
#define XKL_IS_ENGINE_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_ENGINE))
#define XKL_ENGINE_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_ENGINE, XklEngineClass))

#endif				// DOXYGEN_SHOULD_SKIP_THIS


	typedef enum {
/**
 * Group was changed
 */
		GROUP_CHANGED,
/**
 * Indicators were changed
 */
		INDICATORS_CHANGED
	} XklStateChange;

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
	typedef struct {
/** selected group */
		gint32 group;
/** set of active indicators */
		guint32 indicators;
	} XklState;

/**
 *	The main Xklavier engine class
 */
	struct _XklEngine {
/**
 * The superclass object
 */
		GObject parent;
/**
 * Private data
 */
		XklEnginePrivate *priv;
	};

/**
 * The XklEngine class, derived from GObject
 */
	struct _XklEngineClass {
/**
 * The superclass
 */
		GObjectClass parent_class;

/**
 * Used for notifying application of the XKB configuration change.
 */
		void (*config_notify) (XklEngine * engine);

/**
 * Used for notifying application of new window creation (actually, 
 * registration).
 * @param win is a new window
 * @param parent is a new window's parent
 * @return the initial group id for the window (-1 to use the default value)
 */
		 gint(*new_window_notify) (XklEngine * engine, Window win,
					   Window parent);
/**
 * Used for notifying application of the window state change.
 * @param changeType is a mask of changes
 * @param group is a new group
 * @param restore is indicator of whether this state is restored from
 * saved state of set as new.
 */
		void (*state_notify) (XklEngine * engine,
				      XklStateChange changeType,
				      gint group, gboolean restore);

	};


/**
 * Get type info for XklEngine
 * @return GType for XklEngine
 */
	extern GType xkl_engine_get_type(void);


/**
 * Get the instance of the XklEngine. Within a process, there is always once instance
 * @return the singleton instance
 */
	extern XklEngine *xkl_engine_get_instance(Display * display);


/**
 * What kind of backend if used
 * @return some string id of the backend
 */
	extern const gchar *xkl_engine_get_backend_name(XklEngine *
							engine);

/**
 * Provides information regarding available backend features
 * (combination of XKLF_* constants)
 * @return ORed XKLF_* constants
 */
	extern guint xkl_engine_get_features(XklEngine * engine);

/**
 * Provides the information on maximum number of simultaneously supported 
 * groups (layouts)
 * @return maximum number of the groups in configuration, 
 *         0 if no restrictions.
 */
	extern unsigned xkl_engine_get_max_num_groups(XklEngine * engine);

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
	extern gint xkl_engine_start_listen(XklEngine * engine,
					    guint what);

/**
 * Stops listening for XKB-related events
 * @return 0
 */
	extern gint xkl_engine_stop_listen(XklEngine * engine);

/**
 * Temporary pauses listening for XKB-related events
 * @return 0
 */
	extern gint xkl_engine_pause_listen(XklEngine * engine);

/**
 * Resumes listening for XKB-related events
 * @return 0
 */
	extern gint xkl_engine_resume_listen(XklEngine * engine);

/**
 * Grabs some key
 * @param keycode is a keycode
 * @param modifiers is a bitmask of modifiers
 * @return True on success
 */
	extern gboolean xkl_engine_grab_key(XklEngine * engine,
					    gint keycode,
					    unsigned modifiers);

/**
 * Ungrabs some key
 * @param keycode is a keycode
 * @param modifiers is a bitmask of modifiers
 * @return True on success
 */
	extern gboolean xkl_engine_ungrab_key(XklEngine * engine,
					      gint keycode,
					      unsigned modifiers);

/**
 * Processes X events. Should be included into the main event cycle of an
 * application. One of the most important functions. 
 * @param evt is delivered X event
 * @return 0 if the event it processed - 1 otherwise
 * @see xkl_StartListen
 */
	extern gint xkl_engine_events_filter(XklEngine * engine,
					     XEvent * evt);

/**
 * Allows to switch (once) to the secondary group
 */
	extern void
	 xkl_engine_allow_one_switch_to_secondary_group(XklEngine *
							engine);

/** @} */

/**
 * @defgroup currents Current state of the library
 * @{
 */

/**
 * @return currently focused window
 */
	extern Window xkl_engine_get_current_window(XklEngine * engine);

/**
 * @return current state of the keyboard (in XKB terms). 
 * Returned value is a statically allocated buffer, should not be freed.
 */
	extern XklState *xkl_engine_get_current_state(XklEngine * engine);

/** @} */

/**
 * @defgroup wininfo Per-window information
 * @{
 */

/**
 * @return the window title of some window or NULL. 
 * If not NULL, it should be freed with XFree
 */
	extern gchar *xkl_engine_get_window_title(XklEngine * engine,
						  Window w);

/** 
 * Finds the state for a given window (for its "App window").
 * @param win is a target window
 * @param state_out is a structure to store the state
 * @return True on success, otherwise False 
 * (the error message can be obtained using xkl_GetLastError).
 */
	extern gboolean xkl_engine_get_state(XklEngine * engine,
					     Window win,
					     XklState * state_out);

/**
 * Drops the state of a given window (of its "App window").
 * @param win is a target window
 */
	extern void xkl_engine_delete_state(XklEngine * engine,
					    Window win);

/** 
 * Stores ths state for a given window
 * @param win is a target window
 * @param state is a new state of the window
 */
	extern void xkl_engine_save_state(XklEngine * engine, Window win,
					  XklState * state);

/**
 * Sets the "transparent" flag. It means focus switching onto 
 * this window will never change the state.
 * @param win is the window do set the flag for.
 * @param transparent - if true, the windows is transparent.
 * @see xkl_IsTranspatent
 */
	extern void xkl_engine_set_window_transparent(XklEngine * engine,
						      Window win,
						      gboolean
						      transparent);

/**
 * Returns "transparent" flag. 
 * @param win is the window to get the transparent flag from.
 * @see xkl_SetTranspatent
 */
	extern gboolean xkl_engine_is_window_transparent(XklEngine *
							 engine,
							 Window win);

/**
 * Checks whether 2 windows have the same topmost window
 * @param win1 is first window
 * @param win2 is second window
 * @return True is windows are in the same application
 */
	extern gboolean
	    xkl_engine_is_window_from_same_toplevel_window(XklEngine *
							   engine,
							   Window win1,
							   Window win2);

/** @} */

/**
 * @defgroup xkbinfo Various XKB configuration info
 * @{
 */

/**
 * @return the total number of groups in the current XKB configuration 
 * (keyboard)
 */
	extern unsigned xkl_engine_get_num_groups(XklEngine * engine);

/**
 * @return the array of group names for the current XKB configuration 
 * (keyboard).
 * This array is static, should not be freed
 */
	extern const gchar **xkl_engine_get_groups_names(XklEngine *
							 engine);

/**
 * @return the array of indicator names for the current XKB configuration 
 * (keyboard).
 * This array is static, should not be freed
 */
	extern const gchar **xkl_engine_get_indicators_names(XklEngine *
							     engine);

/** @} */

/**
 * @defgroup xkbgroup XKB group calculation and change
 * @{
 */

/**
 * Calculates next group id. Does not change the state of anything.
 * @return next group id
 */
	extern gint xkl_engine_get_next_group(XklEngine * engine);

/**
 * Calculates prev group id. Does not change the state of anything.
 * @return prev group id
 */
	extern gint xkl_engine_get_prev_group(XklEngine * engine);

/**
 * @return saved group id of the current window. 
 * Does not change the state of anything.
 */
	extern gint xkl_engine_get_current_window_group(XklEngine *
							engine);

/**
 * Locks the group. Can be used after xkl_GetXXXGroup functions
 * @param group is a group number for locking
 * @see xkl_GetNextGroup
 * @see xkl_GetPrevGroup
 * @see xkl_GetRestoreGroup
 */
	extern void xkl_engine_lock_group(XklEngine * engine, gint group);

/** @} */

/**
 * @defgroup settings Settings for event processing
 * @{
 */

/**
 * Sets the configuration parameter: group per application
 * @param isGlobal is a new parameter value
 */
	extern void xkl_engine_set_group_per_toplevel_window(XklEngine *
							     engine,
							     gboolean
							     isGlobal);

/**
 *  @return the value of the parameter: group per application
 */
	extern gboolean xkl_engine_is_group_per_toplevel_window(XklEngine *
								engine);

/**
 * Sets the configuration parameter: perform indicators handling
 * @param whetherHandle is a new parameter value
 */
	extern void xkl_engine_set_indicators_handling(XklEngine * engine,
						       gboolean
						       whetherHandle);

/**
 * @return the value of the parameter: perform indicator handling
 */
	extern gboolean xkl_engine_get_indicators_handling(XklEngine *
							   engine);

/**
 * Sets the secondary groups (one bit per group). 
 * Secondary groups require explicit "allowance" for switching
 * @param mask is a new group mask
 * @see xkl_allow_one_switch_to_secondary_group
 */
	extern void xkl_engine_set_secondary_groups_mask(XklEngine *
							 engine,
							 guint mask);

/**
 * @return the secondary group mask
 */
	extern guint xkl_engine_get_secondary_groups_mask(XklEngine *
							  engine);

/**
 * Configures the default group set on window creation.
 * If -1, no default group is used
 * @param group the default group
 */
	extern void xkl_engine_group_set_default(XklEngine * engine,
						 gint group);

/**
 * Returns the default group set on window creation
 * If -1, no default group is used
 * @return the default group
 */
	extern gint xkl_engine_group_get_default(XklEngine * engine);

/** @} */

/**
 * @defgroup debugerr Debugging, error processing 
 * @{
 */

/**
 * @return the text message (statically allocated) of the last error
 */
	extern const gchar *xkl_engine_get_last_error(XklEngine * engine);

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
	extern void _xkl_debug(const gchar file[], const gchar function[],
			       gint level, const gchar format[], ...);

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
	typedef void (*XklLogAppender) (const gchar file[],
					const gchar function[],
					gint level,
					const gchar format[],
					va_list args);

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
	extern void xkl_default_log_appender(const gchar file[],
					     const gchar function[],
					     gint level,
					     const gchar format[],
					     va_list args);

/**
 * Installs the custom log appender.function
 * @param fun is the new log appender
 */
	extern void xkl_set_log_appender(XklLogAppender fun);

/**
 * Sets maximum debug level. 
 * Message of the level more than the one set here - will be ignored
 * @param level is a new debug level
 */
	extern void xkl_set_debug_level(gint level);

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
#endif				/* __cplusplus */
#endif
