#ifndef __XKLAVIER_PRIVATE_XMM_H__
#define __XKLAVIER_PRIVATE_XMM_H__

typedef struct _XmmShortcut 
{
  gint keysym;
  guint modifiers;
} XmmShortcut, *XmmShortcutPtr;

#define MAX_SHORTCUTS_PER_OPTION 4
typedef struct _XmmSwitchOption
{
  const gchar* option_name;
  gint num_shortcuts;
  XmmShortcut shortcuts[MAX_SHORTCUTS_PER_OPTION];
  gint shortcut_steps[MAX_SHORTCUTS_PER_OPTION];
} XmmSwitchOption, *XmmSwitchOptionPtr;

extern gchar* current_xmm_rules;

extern XklConfigRec current_xmm_config;

extern Atom xmm_state_atom;

/* in the ideal world this should be a hashmap */
extern XmmSwitchOption all_switch_options[];

extern void xkl_xmm_grab_ignoring_indicators( gint keycode, guint modifiers );

extern void xkl_xmm_ungrab_ignoring_indicators( gint keycode, guint modifiers );

extern void xkl_xmm_shortcuts_grab( void );

extern void xkl_xmm_shortcuts_ungrab( void );

extern const gchar* xkl_xmm_current_shortcut_get_option_name( void );

XmmSwitchOptionPtr xkl_xmm_current_shortcut_get( void );

extern void xkl_xmm_group_actualize( gint group );

XmmSwitchOptionPtr xkl_xmm_switch_option_find( gint keycode, 
                                               guint state,
                                               gint * current_shortcut_out );

/* Start VTable methods */

extern gboolean xkl_xmm_config_activate( const XklConfigRec * data );

extern void xkl_xmm_config_init( void );

extern gboolean xkl_xmm_config_registry_load( void );

extern gint xkl_xmm_process_x_event( XEvent * kev );

extern void xkl_xmm_free_all_info( void );

extern const gchar **xkl_xmm_groups_get_names( void );

extern unsigned xkl_xmm_groups_get_max_num( void );

extern unsigned xkl_xmm_groups_get_num( void );

extern void xkl_xmm_group_lock( gint group );

extern void xkl_xmm_state_get_real( XklState * current_state_out );

extern gboolean xkl_xmm_if_cached_info_equals_actual( void );

extern gboolean xkl_xmm_load_all_info( void );

extern gint xkl_xmm_listen_pause( void );

extern gint xkl_xmm_listen_resume( void );

/* End of VTable methods */

#endif
