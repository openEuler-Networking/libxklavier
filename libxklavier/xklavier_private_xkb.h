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

extern void xkl_dump_xkb_desc( const char *file_name, 
                               XkbDescPtr kbd );

extern gboolean xkl_xkb_config_multiple_layouts_supported( void );

extern const gchar *xkl_xkb_get_xkb_event_name( gint xkb_type );

extern gboolean xkl_xkb_config_native_prepare( const XklConfigRec *data, 
                                               XkbComponentNamesPtr component_names );

extern void xkl_xkb_config_native_cleanup( XkbComponentNamesPtr component_names );

/* Start VTable methods */

extern gboolean xkl_xkb_config_activate( const XklConfigRec * data );

extern void xkl_xkb_config_init( void );

extern gboolean xkl_xkb_config_load_registry( void );

extern gboolean xkl_xkb_config_write_file( const char *file_name,
                                           const XklConfigRec * data,
                                           const gboolean binary );

extern gint xkl_xkb_event_func( XEvent * kev );

extern void xkl_xkb_free_all_info( void );

extern const gchar **xkl_xkb_groups_get_names( void );

extern gint xkl_xkb_groups_get_max_num( void );

extern gint xkl_xkb_groups_get_num( void );

extern void xkl_xkb_get_server_state( XklState * current_state_out );

extern gboolean xkl_xkb_if_cached_info_equals_actual( void );

extern gboolean xkl_xkb_load_all_info( void );

extern void xkl_xkb_group_lock( gint group );

extern gint xkl_xkb_listen_pause( void );

extern gint xkl_xkb_listen_resume( void );

extern void xkl_xkb_indicators_set( const XklState *window_state );

/* End of VTable methods */

#else

/**
 * VERY VERY BAD STYLE, some kind of 'protected' methods - 
 * but some programs may want to hook into them.
 */
extern gboolean xkl_xkb_config_prepare_native( const XklConfigRec * data, 
                                               gpointer component_names );

extern void xkl_xkb_config_cleanup_native( gpointer component_names );

#endif

extern gboolean xkl_xkb_ext_present;

#endif
