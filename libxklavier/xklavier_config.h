/**
 * @file xklavier_config.h
 */

#ifndef __XKLAVIER_CONFIG_H__
#define __XKLAVIER_CONFIG_H__

#include <glib-object.h>
#include <libxklavier/xklavier.h>

/**
 * Maximum name length, including '\'0' character
 */
#define XKL_MAX_CI_NAME_LENGTH 32

/**
 * Maximum short description length, including '\\0' character.
 * Important: this length is in bytes, so for unicode (UTF-8 encoding in 
 * XML file) the actual maximum length can be smaller.
 */
#define XKL_MAX_CI_SHORT_DESC_LENGTH 10

/**
 * Maximum description length, including '\\0' character.
 * Important: this length is in bytes, so for unicode (UTF-8 encoding in 
 * XML file) the actual maximum length can be smaller.
 */
#define XKL_MAX_CI_DESC_LENGTH 192

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef struct _XklConfig XklConfig;
	typedef struct _XklConfigPrivate XklConfigPrivate;
	typedef struct _XklConfigClass XklConfigClass;

	typedef struct _XklConfigItem XklConfigItem;
	typedef struct _XklConfigItemClass XklConfigItemClass;

	typedef struct _XklConfigRec XklConfigRec;
	typedef struct _XklConfigRecClass XklConfigRecClass;

#define XKL_TYPE_CONFIG             (xkl_config_get_type ())
#define XKL_CONFIG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG, XklConfig))
#define XKL_CONFIG_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG,  XklConfigClass))
#define XKL_IS_CONFIG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG))
#define XKL_IS_CONFIG_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG))
#define XKL_CONFIG_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG, XklConfigClass))

#define XKL_TYPE_CONFIG_ITEM             (xkl_config_item_get_type ())
#define XKL_CONFIG_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItem))
#define XKL_CONFIG_ITEM_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_ITEM,  XklConfigItemClass))
#define XKL_IS_CONFIG_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_IS_CONFIG_ITEM_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_CONFIG_ITEM_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItemClass))

#define XKL_TYPE_CONFIG_REC             (xkl_config_rec_get_type ())
#define XKL_CONFIG_REC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_REC, XklConfigRec))
#define XKL_CONFIG_REC_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_REC,  XklConfigRecClass))
#define XKL_IS_CONFIG_REC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_REC))
#define XKL_IS_CONFIG_REC_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_REC))
#define XKL_CONFIG_REC_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_REC, XklConfigRecClass))

#endif				// DOXYGEN_SHOULD_SKIP_THIS

/**
 * The configuration manager. Corresponds to XML element "configItem".
 */
	struct _XklConfig {
/**
 * The superclass object
 */
		GObject parent;

		XklConfigPrivate *priv;
	};

/**
 * The XklConfig class, derived from GObject
 */
	struct _XklConfigClass {
    /**
     * The superclass
     */
		GObjectClass parent_class;
	};
/**
 * Get type info for XConfigRec
 * @return GType for XConfigRec
 */
	extern GType xkl_config_get_type(void);

/**
 * Create new XklConfigRec
 * @return new instance
 */
	extern XklConfig *xkl_config_get_instance(XklEngine * engine);



/**
 * The configuration item. Corresponds to XML element "configItem".
 */
	struct _XklConfigItem {
/**
 * The superclass object
 */
		GObject parent;
/**
 * The configuration item name. Corresponds to XML element "name".
 */
		gchar name[XKL_MAX_CI_NAME_LENGTH];

/**
 * The configuration item short description. Corresponds to XML element "shortDescription".
 */
		gchar short_description[XKL_MAX_CI_DESC_LENGTH];

/**
 * The configuration item description. Corresponds to XML element "description".
 */
		gchar description[XKL_MAX_CI_DESC_LENGTH];
	};

/**
 * The XklConfigItem class, derived from GObject
 */
	struct _XklConfigItemClass {
    /**
     * The superclass
     */
		GObjectClass parent_class;
	};
/**
 * Get type info for XklConfigItem
 * @return GType for XklConfigItem
 */
	extern GType xkl_config_item_get_type(void);

/**
 * Create new XklConfigItem
 * @return new instance
 */
	extern XklConfigItem *xkl_config_item_new(void);

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
 * @defgroup xklconfiginitterm XKB configuration handling initialization and termination
 * @{
 */

/**
 * Loads XML configuration registry
 * @param file_name file name to load
 * @return true on success
 */
	extern gboolean xkl_config_load_registry_from_file(XklConfig *
							   config,
							   const gchar *
							   file_name);

/**
 * Loads XML configuration registry
 * @return true on success
 */
	extern gboolean xkl_config_load_registry(XklConfig * config);

/**
 * Frees XML configuration registry
 */
	extern void xkl_config_free_registry(XklConfig * config);
/** @} */

/**
 * @defgroup enum XKB configuration elements enumeration functions
 * @{
 */

/**
 * Callback type used for enumerating keyboard models, layouts, variants, options
 * @param item is the item from registry
 * @param data is anything which can be stored into the pointer
 */
	typedef void (*ConfigItemProcessFunc) (const XklConfigItem * item,
					       gpointer data);

/**
 * Callback type used for enumerating keyboard option groups
 * @param item is the item from registry
 * @param allow_multiple_selection is a flag whether this group allows multiple selection
 * @param data is anything which can be stored into the pointer
 */
	typedef void (*GroupProcessFunc) (const XklConfigItem * item,
					  gboolean
					  allow_multiple_selection,
					  gpointer data);
/**
 * Enumerates keyboard models from the XML configuration registry
 * @param func is a callback to call for every model
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_enum_models(XklConfig * config,
					   ConfigItemProcessFunc func,
					   gpointer data);

/**
 * Enumerates keyboard layouts from the XML configuration registry
 * @param func is a callback to call for every layout
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_enum_layouts(XklConfig * config,
					    ConfigItemProcessFunc func,
					    gpointer data);

/**
 * Enumerates keyboard layout variants from the XML configuration registry
 * @param layout_name is the layout name for which variants will be listed
 * @param func is a callback to call for every layout variant
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_enum_layout_variants(XklConfig * config,
						    const gchar *
						    layout_name,
						    ConfigItemProcessFunc
						    func, gpointer data);

/**
 * Enumerates keyboard option groups from the XML configuration registry
 * @param func is a callback to call for every option group
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_enum_option_groups(XklConfig * config,
						  GroupProcessFunc func,
						  gpointer data);

/**
 * Enumerates keyboard options from the XML configuration registry
 * @param option_group_name is the option group name for which variants 
 * will be listed
 * @param func is a callback to call for every option
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_enum_options(XklConfig * config,
					    const gchar *
					    option_group_name,
					    ConfigItemProcessFunc func,
					    gpointer data);

/** @} */

/**
 * @defgroup lookup XKB configuration element lookup functions
 * @{
 */

/**
 * Loads a keyboard model information from the XML configuration registry.
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard model. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_find_model(XklConfig * config,
					      XklConfigItem * item);

/**
 * Loads a keyboard layout information from the XML configuration registry.
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard layout. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_find_layout(XklConfig * config,
					       XklConfigItem * item);

/**
 * Loads a keyboard layout variant information from the XML configuration 
 * registry.
 * @param layout_name is a name of the parent layout
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard layout variant. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_find_variant(XklConfig * config,
						const char *layout_name,
						XklConfigItem * item);

/**
 * Loads a keyboard option group information from the XML configuration 
 * registry.
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard option group. On successfull return, the descriptions are filled.
 * @param allow_multiple_selection is a pointer to some gboolean variable to fill 
 * the corresponding attribute of XML element "group".
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_find_option_group(XklConfig * config,
						     XklConfigItem * item,
						     gboolean *
						     allow_multiple_selection);

/**
 * Loads a keyboard option information from the XML configuration 
 * registry.
 * @param option_group_name is a name of the option group
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard option. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_find_option(XklConfig * config,
					       const gchar *
					       option_group_name,
					       XklConfigItem * item);
/** @} */

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
	extern gboolean xkl_engine_get_names_prop(XklEngine * engine,
						  Atom rules_atom_name,
						  gchar ** rules_file_out,
						  XklConfigRec *
						  config_out);

/**
 * Saves the XKB configuration into any root window property
 * @param rules_atom_name is an atom name of the root window property to write
 * @param rules_file is a rules file name
 * @param config is a configuration to save 
 * @return TRUE on success
 */
	extern gboolean xkl_engine_set_names_prop(XklEngine * engine,
						  Atom rules_atom_name,
						  gchar * rules_file,
						  const XklConfigRec *
						  config);

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
