/**
 * @file xkl_config_rec.h
 */

#ifndef __XKL_CONFIG_REC_H__
#define __XKL_CONFIG_REC_H__

#include <glib-object.h>
#include <libxklavier/xkl_engine.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef struct _XklConfigRec XklConfigRec;
	typedef struct _XklConfigRecClass XklConfigRecClass;

#define XKL_TYPE_CONFIG_REC             (xkl_config_rec_get_type ())
#define XKL_CONFIG_REC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_REC, XklConfigRec))
#define XKL_CONFIG_REC_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_REC,  XklConfigRecClass))
#define XKL_IS_CONFIG_REC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_REC))
#define XKL_IS_CONFIG_REC_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_REC))
#define XKL_CONFIG_REC_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_REC, XklConfigRecClass))

#endif				// DOXYGEN_SHOULD_SKIP_THIS

/**
 * Basic configuration params
 */
	struct _XklConfigRec {
/**
 * The superclass object
 */
		GObject parent;
/**
 * The keyboard model
 */
		gchar *model;
/**
 * The array of keyboard layouts
 */
		gchar **layouts;
/**
 * The array of keyboard layout variants (if any)
 */
		gchar **variants;
/**
 * The array of keyboard layout options
 */
		gchar **options;
	};

/**
 * The XklConfigRec class, derived from GObject
 */
	struct _XklConfigRecClass {
    /**
     * The superclass
     */
		GObjectClass parent_class;
	};

/**
 * Get type info for XConfigRec
 * @return GType for XConfigRec
 */
	extern GType xkl_config_rec_get_type(void);

/**
 * Create new XklConfigRec
 * @return new instance
 */
	extern XklConfigRec *xkl_config_rec_new(void);

/**
 * @defgroup activation XKB configuration activation
 * @{
 */

/**
 * Activates some XKB configuration
 * @param data is a valid XKB configuration
 * description. Can be NULL
 * @return TRUE on success
 * @see XklSetKeyAsSwitcher
 * At the moment, accepts only _ONE_ layout. Later probably I'll improve this..
 */
	extern gboolean xkl_config_rec_activate(const XklConfigRec * data,
						XklEngine * engine);

/**
 * Loads the current XKB configuration (from X server)
 * @param data is a buffer for XKB configuration
 * @return TRUE on success
 */
	extern gboolean xkl_config_rec_get_from_server(XklConfigRec * data,
						       XklEngine * engine);

/**
 * Loads the current XKB configuration (from backup)
 * @param data is a buffer for XKB configuration
 * @return TRUE on success
 * @see XklBackupNamesProp
 */
	extern gboolean xkl_config_rec_get_from_backup(XklConfigRec * data,
						       XklEngine * engine);

/**
 * Writes some XKB configuration into XKM/XKB file
 * @param file_name is a name of the file to create
 * @param data is a valid XKB configuration
 * description. Can be NULL
 * @param binary is a flag indicating whether the output file should be binary
 * @return TRUE on success
 */
	extern gboolean xkl_config_rec_write_to_file(XklEngine * engine,
						     const gchar *
						     file_name,
						     const XklConfigRec *
						     data,
						     const gboolean
						     binary);

/** @} */

/**
 * @defgroup props Saving and restoring XKB configuration into X root window properties
 * Generalizes XkbRF_GetNamesProp and XkbRF_SetNamesProp.
 * @{
 */

/**
 * Gets the XKB configuration from any root window property
 * @param rules_atom_name is an atom name of the root window property to read
 * @param rules_file_out is a pointer to hold the file name
 * @param config_out is a buffer to hold the result - 
 *   all records are allocated using standard malloc 
 * @return TRUE on success
 */
	extern gboolean
	    xkl_config_rec_get_from_root_window_property(XklConfigRec *
							 config_out,
							 Atom
							 rules_atom_name,
							 gchar **
							 rules_file_out,
							 XklEngine *
							 engine);

/**
 * Saves the XKB configuration into any root window property
 * @param rules_atom_name is an atom name of the root window property to write
 * @param rules_file is a rules file name
 * @param config is a configuration to save 
 * @return TRUE on success
 */
	extern gboolean xkl_config_rec_set_to_root_window_property(const
								   XklConfigRec
								   *
								   config,
								   Atom
								   rules_atom_name,
								   gchar *
								   rules_file,
								   XklEngine
								   *
								   engine);

/**
 * Backups current XKB configuration into some property - 
 * if this property is not defined yet.
 * @return TRUE on success
 */
	extern gboolean xkl_backup_names_prop(XklEngine * engine);

/**
 * Restores XKB from the property saved by xkl_backup_names_prop
 * @return TRUE on success
 * @see xkl_backup_names_prop
 */
	extern gboolean xkl_restore_names_prop(XklEngine * engine);

/** @} */

/**
 * @defgroup xklconfig XklConfigRec management utilities
 * Little utilities for managing XklConfigRec.
 * @{
 */

/**
 * Resets the record (equal to Destroy and Init)
 * @param data is a record to reset
 */
	extern void xkl_config_rec_reset(XklConfigRec * data);

/**
 * Compares the records
 * @param data1 is a record to compare
 * @param data2 is another record
 * @return TRUE if records are same
 */
	extern gboolean xkl_config_rec_equals(XklConfigRec * data1,
					      XklConfigRec * data2);

/** @} */

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
