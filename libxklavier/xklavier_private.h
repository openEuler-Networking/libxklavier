#ifndef __XKLAVIER_PRIVATE_H__
#define __XKLAVIER_PRIVATE_H__

#include <stdio.h>

#include <libxml/xpath.h>

#include <libxklavier/xklavier_config.h>

enum { WM_NAME,
	WM_STATE,
	XKLAVIER_STATE,
	XKLAVIER_TRANSPARENT,
	XKLAVIER_ALLOW_SECONDARY,
	TOTAL_ATOMS
};

struct _XklEnginePrivate {

	gboolean group_per_toplevel_window;

	gboolean handle_indicators;

	gboolean skip_one_restore;

	gint default_group;

	guint listener_type;

	guint secondary_groups_mask;

	Window root_window;

	Window prev_toplvl_win;

	Window curr_toplvl_win;

	XErrorHandler default_error_handler;

	Status last_error_code;

	const gchar *last_error_message;

	XklState curr_state;

	Atom atoms[TOTAL_ATOMS];

	Display *display;

  /**
   * Backend name
   */
	const gchar *backend_id;

  /**
   * Functions supported by the backend, combination of XKLF_* constants
   */
	guint8 features;

  /**
   * Activates the configuration.
   * xkb: create proper the XkbDescRec and send it to the server
   * xmodmap: save the property, init layout #1
   */
	 gboolean(*activate_config) (XklConfig * config,
				     const XklConfigRec * data);

  /**
   * Background-specific initialization.
   * xkb: XkbInitAtoms - init internal xkb atoms table
   * xmodmap: void.
   */
	void (*init_config) (XklConfig * config);

  /**
   * Loads the registry tree into DOM (using whatever path(s))
   * The XklVTConfigFreeRegistry is static - no virtualization necessary.
   * xkb: loads xml from XKB_BASE+"/rules/"+ruleset+".xml"
   * xmodmap: loads xml from XMODMAP_BASE+"/"+ruleset+".xml"
   */
	 gboolean(*load_config_registry) (XklConfig * config);

  /**
   * Write the configuration into the file (binary/textual)
   * xkb: write xkb or xkm file
   * xmodmap: if text requested, just dump XklConfigRec to the 
   * file - not really useful. If binary - fail (not supported)
   */
	 gboolean(*write_config_to_file) (XklConfig * config,
					  const gchar * file_name,
					  const XklConfigRec * data,
					  const gboolean binary);

  /**
   * Get the list of the group names
   * xkb: return cached list of the group names
   * xmodmap: return the list of layouts from the internal XklConfigRec
   */
	const gchar **(*get_groups_names) (XklEngine * engine);

  /**
   * Get the maximum number of loaded groups
   * xkb: returns 1 or XkbNumKbdGroups
   * xmodmap: return 0
   */
	 guint(*get_max_num_groups) (XklEngine * engine);

  /**
   * Get the number of loaded groups
   * xkb: return from the cached XkbDesc
   * xmodmap: return number of layouts from internal XklConfigRec
   */
	 guint(*get_num_groups) (XklEngine * engine);

  /**
   * Switches the keyboard to the group N
   * xkb: simple one-liner to call the XKB function
   * xmodmap: changes the root window property 
   * (listener invokes xmodmap with appropriate config file).
   */
	void (*lock_group) (XklEngine * engine, gint group);

  /**
   * Handles X events.
   * xkb: XkbEvent handling
   * xmodmap: keep track on the root window properties. What else can we do?
   */
	 gint(*process_x_event) (XklEngine * engine, XEvent * xev);

  /**
   * Flushes the cached server config info.
   * xkb: frees XkbDesc
   * xmodmap: frees internal XklConfigRec
   */
	void (*free_all_info) (XklEngine * engine);

  /**
   * Compares the cached info with the actual one, from the server
   * xkb: Compares some parts of XkbDescPtr
   * xmodmap: returns False
   */
	 gboolean(*if_cached_info_equals_actual) (XklEngine * engine);

  /**
   * Loads the configuration info from the server
   * xkb: loads XkbDesc, names, indicators
   * xmodmap: loads internal XklConfigRec from server
   */
	 gboolean(*load_all_info) (XklEngine * engine);

  /**
   * Gets the current state
   * xkb: XkbGetState and XkbGetIndicatorState
   * xmodmap: check the root window property (regarding the group)
   */
	void (*get_server_state) (XklEngine * engine,
				  XklState * current_state_out);

  /**
   * Stop tracking the keyboard-related events
   * xkb: XkbSelectEvents(..., 0)
   * xmodmap: Ungrab the switching shortcut.
   */
	 gint(*pause_listen) (XklEngine * engine);

  /**
   * Start tracking the keyboard-related events
   * xkb: XkbSelectEvents + XkbSelectEventDetails
   * xmodmap: Grab the switching shortcut.
   */
	 gint(*resume_listen) (XklEngine * engine);

  /**
   * Set the indicators state from the XklState
   * xkb: XklSetIndicator for all indicators
   * xmodmap: NULL. Not supported
   */
	void (*set_indicators) (XklEngine * engine,
				const XklState * window_state);

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
	const gchar *default_model;

  /**
   * Fallback for missing layout
   */
	const gchar *default_layout;

};

extern XklEngine *xkl_get_the_engine(void);

struct _XklConfigPrivate {
	XklEngine *engine;

	xmlDocPtr doc;

	xmlXPathContextPtr xpath_context;
};

extern void xkl_engine_ensure_vtable_inited(XklEngine * engine);

extern void xkl_engine_process_focus_in_evt(XklEngine * engine,
					    XFocusChangeEvent * fev);
extern void xkl_engine_process_focus_out_evt(XklEngine * engine,
					     XFocusChangeEvent * fev);
extern void xkl_engine_process_property_evt(XklEngine * engine,
					    XPropertyEvent * rev);
extern void xkl_engine_process_create_window_evt(XklEngine * engine,
						 XCreateWindowEvent * cev);

extern void xkl_process_error(Display * dpy, XErrorEvent * evt);

extern void xkl_engine_process_state_modification(XklEngine * engine,
						  XklStateChange
						  change_type, gint group,
						  unsigned inds,
						  gboolean set_indicators);

extern Window xkl_get_registered_parent(Window win);
extern gboolean xkl_engine_load_all_info(XklEngine * engine);
extern void xkl_engine_free_all_info(XklEngine * engine);
extern void xkl_engine_reset_all_info(XklEngine * engine,
				      const gchar reason[]);
extern gboolean xkl_engine_load_window_tree(XklEngine * engine);
extern gboolean xkl_engine_load_subtree(XklEngine * engine, Window window,
					gint level, XklState * init_state);

extern gboolean xkl_engine_if_window_has_wm_state(XklEngine * engine,
						  Window win);


/**
 * Toplevel window stuff
 */
extern void xkl_engine_add_toplevel_window(XklEngine * engine, Window win,
					   Window parent, gboolean force,
					   XklState * init_state);

extern gboolean xkl_engine_find_toplevel_window_bottom_to_top(XklEngine *
							      engine,
							      Window win,
							      Window *
							      toplevel_win_out);

extern gboolean xkl_engine_find_toplevel_window(XklEngine * engine,
						Window win,
						Window * toplevel_win_out);

extern gboolean xkl_engine_is_toplevel_window_transparent(XklEngine *
							  engine,
							  Window
							  toplevel_win);

extern void xkl_engine_set_toplevel_window_transparent(XklEngine * engine,
						       Window toplevel_win,
						       gboolean
						       transparent);

extern gboolean xkl_engine_get_toplevel_window_state(XklEngine * engine,
						     Window toplevel_win,
						     XklState * state_out);

extern void xkl_engine_remove_toplevel_window_state(XklEngine * engine,
						    Window toplevel_win);
extern void xkl_engine_save_toplevel_window_state(XklEngine * engine,
						  Window toplevel_win,
						  XklState * state);
/***/

extern void xkl_engine_select_input_merging(XklEngine * engine, Window win,
					    gulong mask);

extern gchar *xkl_get_debug_window_title(Window win);

extern Status xkl_engine_query_tree(XklEngine * engine,
				    Window w,
				    Window * root_out,
				    Window * parent_out,
				    Window ** children_out,
				    guint * nchildren_out);

extern gboolean xkl_engine_set_indicator(XklEngine * engine,
					 gint indicator_num, gboolean set);

extern void xkl_engine_try_call_state_func(XklEngine * engine,
					   XklStateChange change_type,
					   XklState * old_state);

extern void xkl_i18n_init(void);

extern gchar *xkl_locale_from_utf8(const gchar * utf8string);

extern gint xkl_get_language_priority(const gchar * language);

extern gchar *xkl_engine_get_ruleset_name(XklEngine * engine,
					  const gchar default_ruleset[]);

extern gboolean xkl_config_rec_get_full_from_server(gchar **
						    rules_file_out,
						    XklConfigRec * data,
						    XklEngine * engine);

extern gchar *xkl_strings_concat_comma_separated(gchar ** array);

extern void xkl_strings_split_comma_separated(gchar *** array,
					      const gchar * merged);

/**
 * XConfigRec
 */
extern gchar *xkl_config_rec_merge_layouts(const XklConfigRec * data);

extern gchar *xkl_config_rec_merge_variants(const XklConfigRec * data);

extern gchar *xkl_config_rec_merge_options(const XklConfigRec * data);

extern void xkl_config_rec_split_layouts(XklConfigRec * data,
					 const gchar * merged);

extern void xkl_config_rec_split_variants(XklConfigRec * data,
					  const gchar * merged);

extern void xkl_config_rec_split_options(XklConfigRec * data,
					 const gchar * merged);
/***/

extern void xkl_config_dump(FILE * file, XklConfigRec * data);

extern const gchar *xkl_event_get_name(gint type);

extern void xkl_engine_update_current_state(XklEngine * engine, gint group,
					    unsigned indicators,
					    const gchar reason[]);

extern gint xkl_xkb_init(XklEngine * engine);

extern gint xkl_xmm_init(XklEngine * engine);

extern gboolean
xkl_engine_is_one_switch_to_secondary_group_allowed(XklEngine * engine);

extern void xkl_engine_one_switch_to_secondary_group_performed(XklEngine *
							       engine);

#define XKLAVIER_STATE_PROP_LENGTH 2

/* taken from XFree86 maprules.c */
#define XKB_RF_NAMES_PROP_MAXLEN 1024

#define WINID_FORMAT "%lx"

#define xkl_engine_get_display(engine) ((engine)->priv->display)
#define xkl_engine_vcall(engine,func)  (*(engine)->priv->func)
#define xkl_config_get_engine(config) ((config)->priv->engine)

extern gint xkl_debug_level;

#endif
