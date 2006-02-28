#ifndef __XKLAVIER_PRIVATE_XMM_H__
#define __XKLAVIER_PRIVATE_XMM_H__

typedef struct _XmmShortcut {
	guint keysym;
	guint modifiers;
} XmmShortcut;

#define MAX_SHORTCUTS_PER_OPTION 4
typedef struct _XmmSwitchOption {
	const gchar *option_name;
	XmmShortcut shortcuts[MAX_SHORTCUTS_PER_OPTION + 1];
	gint shortcut_steps[MAX_SHORTCUTS_PER_OPTION + 1];
} XmmSwitchOption;

extern gchar *current_xmm_rules;

extern XklConfigRec current_xmm_config;

extern Atom xmm_state_atom;

/* in the ideal world this should be a hashmap */
extern XmmSwitchOption all_switch_options[];

extern void xkl_xmm_grab_ignoring_indicators(XklEngine * engine,
					     gint keycode,
					     guint modifiers);

extern void xkl_xmm_ungrab_ignoring_indicators(XklEngine * engine,
					       gint keycode,
					       guint modifiers);

extern void xkl_xmm_shortcuts_grab(XklEngine * engine);

extern void xkl_xmm_shortcuts_ungrab(XklEngine * engine);

extern const gchar *xkl_xmm_shortcut_get_current_option_name(void);

XmmSwitchOption *xkl_xmm_shortcut_get_current(void);

extern void xkl_xmm_actualize_group(XklEngine * engine, gint group);

const XmmSwitchOption *xkl_xmm_find_switch_option(XklEngine * engine,
						  gint keycode,
						  guint state,
						  gint *
						  current_shortcut_out);

/* Start VTable methods */

extern gboolean xkl_xmm_activate_config(XklConfig * config,
					const XklConfigRec * data);

extern void xkl_xmm_init_config(XklConfig * config);

extern gboolean xkl_xmm_load_config_registry(XklConfig * config);

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

/* End of VTable methods */

#endif
