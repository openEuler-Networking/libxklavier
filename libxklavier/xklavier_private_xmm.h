#ifndef __XKLAVIER_PRIVATE_XMM_H__
#define __XKLAVIER_PRIVATE_XMM_H__

typedef struct _XmmShortcut {
	guint keysym;
	guint modifiers;
} XmmShortcut;

#define MAX_SHORTCUTS_PER_OPTION 4
typedef struct _XmmSwitchOption {
	XmmShortcut shortcuts[MAX_SHORTCUTS_PER_OPTION + 1];
	gint shortcut_steps[MAX_SHORTCUTS_PER_OPTION + 1];
} XmmSwitchOption;

typedef struct _XklXmm {
	gchar *current_rules;

	XklConfigRec current_config;

	Atom state_atom;

	GHashTable *switch_options;
} XklXmm;

/* in the ideal world this should be a hashmap */
extern void xkl_xmm_grab_ignoring_indicators(XklEngine * engine,
					     gint keycode,
					     guint modifiers);

extern void xkl_xmm_ungrab_ignoring_indicators(XklEngine * engine,
					       gint keycode,
					       guint modifiers);

extern void xkl_xmm_shortcuts_grab(XklEngine * engine);

extern void xkl_xmm_shortcuts_ungrab(XklEngine * engine);

extern const gchar *xkl_xmm_shortcut_get_current_option_name(XklEngine *
							     engine);

XmmSwitchOption *xkl_xmm_shortcut_get_current(XklEngine * engine);

extern void xkl_xmm_actualize_group(XklEngine * engine, gint group);

const XmmSwitchOption *xkl_xmm_find_switch_option(XklEngine * engine,
						  gint keycode,
						  guint state,
						  gint *
						  current_shortcut_out);

extern void xkl_xmm_init_switch_options(XklXmm * xmm);

extern void xkl_xmm_term_switch_options(XklXmm * xmm);
/* Start VTable methods */

extern gboolean xkl_xmm_activate_config_rec(XklEngine * engine,
					    const XklConfigRec * data);

extern void xkl_xmm_init_config_registry(XklConfigRegistry * config);

extern gboolean xkl_xmm_load_config_registry(XklConfigRegistry * config);

extern gint xkl_xmm_process_x_event(XklEngine * engine, XEvent * kev);

extern void xkl_xmm_free_all_info(XklEngine * engine);

extern const gchar **xkl_xmm_get_groups_names(XklEngine * engine);

extern guint xkl_xmm_get_max_num_groups(XklEngine * engine);

extern guint xkl_xmm_get_num_groups(XklEngine * engine);

extern void xkl_xmm_lock_group(XklEngine * engine, gint group);

extern void xkl_xmm_get_server_state(XklEngine * engine,
				     XklState * current_state_out);

extern gboolean xkl_xmm_if_cached_info_equals_actual(XklEngine * engine);

extern gboolean xkl_xmm_load_all_info(XklEngine * engine);

extern gint xkl_xmm_listen_pause(XklEngine * engine);

extern gint xkl_xmm_listen_resume(XklEngine * engine);

extern void xkl_xmm_set_indicators(XklEngine * engine,
				   const XklState * window_state);

extern void xkl_xmm_term(XklEngine * engine);

/* End of VTable methods */

#endif
