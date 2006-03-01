#ifndef __XKLAVIER_PRIVATE_XKB_H__
#define __XKLAVIER_PRIVATE_XKB_H__

#ifdef XKB_HEADERS_PRESENT

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#define ForPhysIndicators( i, bit ) \
    for ( i=0, bit=1; i<XkbNumIndicators; i++, bit<<=1 ) \
          if ( xkl_xkb_desc->indicators->phys_indicators & bit )

extern gint xkl_xkb_event_type, xkl_xkb_error_code;

extern XkbRF_VarDefsRec xkl_var_defs;

extern XkbDescPtr xkl_xkb_desc;

extern gchar *xkl_xkb_indicator_names[];

extern void xkl_engine_dump_xkb_desc(XklEngine * engine,
				     const char *file_name,
				     XkbDescPtr kbd);

extern gboolean xkl_xkb_multiple_layouts_supported(XklEngine * engine);

extern const gchar *xkl_xkb_event_get_name(gint xkb_type);

extern gboolean xkl_xkb_config_native_prepare(XklEngine * engine,
					      const XklConfigRec * data,
					      XkbComponentNamesPtr
					      component_names);

extern void xkl_xkb_config_native_cleanup(XklEngine * engine,
					  XkbComponentNamesPtr
					  component_names);

/* Start VTable methods */

extern gboolean xkl_xkb_activate_config(XklEngine * engine,
					const XklConfigRec * data);

extern void xkl_xkb_init_config(XklConfig * config);

extern gboolean xkl_xkb_load_config_registry(XklConfig * config);

extern gboolean xkl_xkb_write_config_to_file(XklEngine * engine,
					     const char *file_name,
					     const XklConfigRec * data,
					     const gboolean binary);

extern gint xkl_xkb_process_x_event(XklEngine * engine, XEvent * kev);

extern void xkl_xkb_free_all_info(XklEngine * engine);

extern const gchar **xkl_xkb_get_groups_names(XklEngine * engine);

extern guint xkl_xkb_get_max_num_groups(XklEngine * engine);

extern guint xkl_xkb_get_num_groups(XklEngine * engine);

extern void xkl_xkb_get_server_state(XklEngine * engine,
				     XklState * current_state_out);

extern gboolean xkl_xkb_if_cached_info_equals_actual(XklEngine * engine);

extern gboolean xkl_xkb_load_all_info(XklEngine * engine);

extern void xkl_xkb_lock_group(XklEngine * engine, gint group);

extern gint xkl_xkb_pause_listen(XklEngine * engine);

extern gint xkl_xkb_resume_listen(XklEngine * engine);

extern void xkl_xkb_set_indicators(XklEngine * engine,
				   const XklState * window_state);

/* End of VTable methods */

#else

/**
 * VERY VERY BAD STYLE, some kind of 'protected' methods - 
 * but some programs may want to hook into them.
 */
extern gboolean xkl_xkb_config_prepare_native(const XklConfigRec * data,
					      gpointer component_names);

extern void xkl_xkb_config_cleanup_native(gpointer component_names);

#endif

extern gboolean xkl_xkb_ext_present;

#endif
