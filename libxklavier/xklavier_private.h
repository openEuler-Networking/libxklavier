#ifndef __XKLAVIER_PRIVATE_H__
#define __XKLAVIER_PRIVATE_H__

#include <stdio.h>

#include <libxklavier/xklavier_config.h>

/* XklConfigRec */
typedef gboolean ( *XklVTConfigActivateFunc )( const XklConfigRec * data );

typedef void ( *XklVTConfigInitFunc )( void );

typedef gboolean ( *XklVTConfigLoadRegistryFunc )( void );

typedef gboolean ( *XklVTConfigWriteFileFunc )( const gchar *file_name,
                                                const XklConfigRec * data,
                                                const gboolean binary );

/* Groups */
typedef const gchar **( *XklVTGroupsGetNamesFunc )( void );

typedef gint ( *XklVTGroupsGetMaxNumFunc )( void );

typedef gint ( *XklVTGroupsGetNumFunc )( void );

typedef void ( *XklVTGroupLockFunc )( gint group );


typedef gint ( *XklVTEventFunc )( XEvent *xev );

typedef void ( *XklVTFreeAllInfoFunc )( void );

typedef gboolean ( *XklVTIfCachedInfoEqualsActualFunc) ( void );

typedef gboolean ( *XklVTLoadAllInfoFunc )( void );

typedef void ( *XklVTGetServerStateFunc)( XklState * current_state_out );

typedef gint ( *XklVTPauseResumeListenFunc )( void );

typedef void ( *XklVTIndicatorsSetFunc )( const XklState *window_state );

typedef struct
{
  /**
   * Backend name
   */
  const gchar *id;

  /**
   * Functions supported by the backend, combination of XKLF_* constants
   */
  guint8 features;

  /**
   * Activates the configuration.
   * xkb: create proper the XkbDescRec and send it to the server
   * xmodmap: save the property, init layout #1
   */
  XklVTConfigActivateFunc config_activate_func;

  /**
   * Background-specific initialization.
   * xkb: XkbInitAtoms - init internal xkb atoms table
   * xmodmap: void.
   */
  XklVTConfigInitFunc config_init_func; /* private */

  /**
   * Loads the registry tree into DOM (using whatever path(s))
   * The XklVTConfigFreeRegistry is static - no virtualization necessary.
   * xkb: loads xml from XKB_BASE+"/rules/"+ruleset+".xml"
   * xmodmap: loads xml from XMODMAP_BASE+"/"+ruleset+".xml"
   */
  XklVTConfigLoadRegistryFunc config_load_registry_func;

  /**
   * Write the configuration into the file (binary/textual)
   * xkb: write xkb or xkm file
   * xmodmap: if text requested, just dump XklConfigRec to the 
   * file - not really useful. If binary - fail (not supported)
   */
  XklVTConfigWriteFileFunc config_write_file_func;


  /**
   * Get the list of the group names
   * xkb: return cached list of the group names
   * xmodmap: return the list of layouts from the internal XklConfigRec
   */
  XklVTGroupsGetNamesFunc groups_get_names_func;

  /**
   * Get the maximum number of loaded groups
   * xkb: returns 1 or XkbNumKbdGroups
   * xmodmap: return 0
   */
  XklVTGroupsGetMaxNumFunc groups_get_max_num_func;

  /**
   * Get the number of loaded groups
   * xkb: return from the cached XkbDesc
   * xmodmap: return number of layouts from internal XklConfigRec
   */
  XklVTGroupsGetNumFunc groups_get_num_func;

  /**
   * Switches the keyboard to the group N
   * xkb: simple one-liner to call the XKB function
   * xmodmap: changes the root window property 
   * (listener invokes xmodmap with appropriate config file).
   */
  XklVTGroupLockFunc group_lock_func;


  /**
   * Handles X events.
   * xkb: XkbEvent handling
   * xmodmap: keep track on the root window properties. What else can we do?
   */
  XklVTEventFunc event_func;

  /**
   * Flushes the cached server config info.
   * xkb: frees XkbDesc
   * xmodmap: frees internal XklConfigRec
   */
  XklVTFreeAllInfoFunc free_all_info_func; /* private */

  /**
   * Compares the cached info with the actual one, from the server
   * xkb: Compares some parts of XkbDescPtr
   * xmodmap: returns False
   */
  XklVTIfCachedInfoEqualsActualFunc if_cached_info_equals_actual_func;

  /**
   * Loads the configuration info from the server
   * xkb: loads XkbDesc, names, indicators
   * xmodmap: loads internal XklConfigRec from server
   */
  XklVTLoadAllInfoFunc load_all_info_func; /* private */

  /**
   * Gets the current stateCallback
   * xkb: XkbGetState and XkbGetIndicatorState
   * xmodmap: check the root window property (regarding the group)
   */
  XklVTGetServerStateFunc get_server_state_func;

  /**
   * Stop tracking the keyboard-related events
   * xkb: XkbSelectEvents(..., 0)
   * xmodmap: Ungrab the switching shortcut.
   */
  XklVTPauseResumeListenFunc listen_pause_func;

  /**
   * Start tracking the keyboard-related events
   * xkb: XkbSelectEvents + XkbSelectEventDetails
   * xmodmap: Grab the switching shortcut.
   */
  XklVTPauseResumeListenFunc listen_resume_func;

  /**
   * Set the indicators state from the XklState
   * xkb: XklSetIndicator for all indicators
   * xmodmap: NULL. Not supported
   */
  XklVTIndicatorsSetFunc indicators_set_func; /* private */
  
  /* all data is private - no direct access */
  /**
   * The base configuration atom.
   * xkb: _XKB_RF_NAMES_PROP_ATOM
   * xmodmap:  "_XMM_NAMES"
   */
  Atom base_config_atom;
  
  /**
   * The configuration backup atom
   * xkb: "_XKB_RULES_NAMES_BACKUP"
   * xmodmap: "_XMM_NAMES_BACKUP"
   */
  Atom backup_config_atom;
  
  /**
   * Fallback for missing model
   */
  const gchar* default_model;

  /**
   * Fallback for missing layout
   */
  const gchar* default_layout;
  
} XklVTable;

extern void xkl_ensure_vtable_inited( void );

extern void xkl_process_focus_in_evt( XFocusChangeEvent *fev );
extern void xkl_process_focus_out_evt( XFocusChangeEvent *fev );
extern void xkl_process_property_evt( XPropertyEvent *rev );
extern void xkl_process_create_window_evt( XCreateWindowEvent *cev );

extern void xkl_process_error( Display *dpy, XErrorEvent *evt );

extern void xkl_process_state_modification( XklStateChange change_type, 
                                            gint group, 
                                            unsigned inds,
                                            gboolean set_indicators );

extern Window xkl_get_registered_parent( Window win );
extern gboolean xkl_load_all_info( void );
extern void xkl_free_all_info( void );
extern void xkl_reset_all_info( const gchar reason[] );
extern gboolean xkl_load_window_tree( void );
extern gboolean xkl_load_subtree( Window window, 
                                  gint level, 
                                  XklState *init_state );

extern gboolean xkl_has_wm_state( Window win );


/**
 * Toplevel window stuff
 */
extern void xkl_toplevel_window_add( Window win, 
                                     Window parent, gboolean force,
                                     XklState *init_state );

extern gboolean xkl_toplevel_window_find_bottom_to_top( Window win, 
                                                       Window *toplevel_win_out );

extern gboolean xkl_toplevel_window_find( Window win, 
                                         Window *toplevel_win_out );

extern gboolean xkl_toplevel_window_is_transparent( Window toplevel_win );

extern void xkl_toplevel_window_set_transparent( Window toplevel_win,
                                                 gboolean transparent );

extern gboolean xkl_toplevel_window_get_state( Window toplevel_win, 
                                               XklState *state_out );

extern void xkl_toplevel_window_remove_state( Window toplevel_win );
extern void xkl_toplevel_window_save_state( Window toplevel_win, XklState *state );
/***/

extern void xkl_select_input_merging( Window win, gulong mask );

extern gchar *xkl_get_debug_window_title( Window win );

extern Status xkl_status_query_tree( Display * display,
                                     Window w,
                                     Window * root_out,
                                     Window * parent_out,
                                     Window ** children_out,
                                     guint *nchildren_out );

extern gboolean xkl_indicator_set( gint indicator_num, gboolean set );

extern void xkl_try_call_state_func( XklStateChange change_type,
                                     XklState * old_state );

extern void xkl_i18n_init( void );

extern gchar *xkl_locale_from_utf8( const gchar *utf8string );

extern gint xkl_get_language_priority( const gchar *language );

extern gchar* xkl_rules_set_get_name( const gchar default_ruleset[] );

extern gboolean xkl_config_get_full_from_server( gchar **rules_file_out,
                                                 XklConfigRec * data );

extern gchar *xkl_strings_concat_comma_separated( gchar **array );

extern void xkl_strings_split_comma_separated( gchar ***array,
                                               const gchar *merged );

/**
 * XConfigRec
 */
extern gchar *xkl_config_rec_merge_layouts( const XklConfigRec * data );

extern gchar *xkl_config_rec_merge_variants( const XklConfigRec * data );

extern gchar *xkl_config_rec_merge_options( const XklConfigRec * data );

extern void xkl_config_rec_split_layouts( XklConfigRec * data,
                                          const gchar *merged );

extern void xkl_config_rec_split_variants( XklConfigRec * data,
                                           const gchar *merged );

extern void xkl_config_rec_split_options( XklConfigRec * data,
                                          const gchar *merged );
/***/

extern void xkl_config_dump( FILE* file,
                             XklConfigRec * data );
                           
extern const gchar *xkl_get_event_name( gint type );

extern void xkl_update_current_state( gint group, 
                                      unsigned indicators, 
                                      const gchar reason[] );

extern gint xkl_xkb_init( void );

extern gint xkl_xmm_init( void );

extern gboolean xkl_is_one_switch_to_secondary_group_allowed( void );

extern void xkl_one_switch_to_secondary_group_performed( void );

extern Display *xkl_display;

extern Window xkl_root_window;

extern XklState xkl_current_state;

extern Window xkl_current_client;

extern Status xkl_last_error_code;

extern const gchar *xkl_last_error_message;

extern XErrorHandler xkl_default_error_handler;

extern gchar *xkl_indicator_names[];

enum { WM_NAME,
  WM_STATE,
  XKLAVIER_STATE,
  XKLAVIER_TRANSPARENT,
  XKLAVIER_ALLOW_SECONDARY,
  TOTAL_ATOMS };

#define XKLAVIER_STATE_PROP_LENGTH 2

/* taken from XFree86 maprules.c */
#define XKB_RF_NAMES_PROP_MAXLEN 1024

extern Atom xkl_atoms[];

extern gboolean xkl_allow_secondary_group_once;

extern gint xkl_default_group;

extern gboolean xkl_skip_one_restore;

extern guint xkl_secondary_groups_mask;

extern gint xkl_debug_level;

extern guint xkl_listener_type;

extern Window xkl_toplevel_window_prev;

#define WINID_FORMAT "%lx"

extern XklConfigNotifyFunc xkl_config_callback;

extern gpointer xkl_config_callback_data;

extern XklNewWindowNotifyFunc xkl_new_window_callback;

extern gpointer xkl_new_window_callback_data;

extern XklVTable *xkl_vtable;

#endif
