/**
 * @file xkl_config_item.h
 */

#ifndef __XKL_CONFIG_ITEM_H__
#define __XKL_CONFIG_ITEM_H__

#include <glib-object.h>

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

	typedef struct _XklConfigItem XklConfigItem;
	typedef struct _XklConfigItemClass XklConfigItemClass;

#define XKL_TYPE_CONFIG_ITEM             (xkl_config_item_get_type ())
#define XKL_CONFIG_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItem))
#define XKL_CONFIG_ITEM_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_ITEM,  XklConfigItemClass))
#define XKL_IS_CONFIG_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_IS_CONFIG_ITEM_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_CONFIG_ITEM_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItemClass))

#endif				// DOXYGEN_SHOULD_SKIP_THIS

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

/** @} */

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
