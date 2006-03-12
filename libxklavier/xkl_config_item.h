#ifndef __XKL_CONFIG_ITEM_H__
#define __XKL_CONFIG_ITEM_H__

#include <glib-object.h>

/*
 * Maximum name length, including '\'0' character
 */
#define XKL_MAX_CI_NAME_LENGTH 32

/*
 * Maximum short description length, including '\\0' character
 * (this length is in bytes, so for UTF-8 encoding in 
 * XML file the actual maximum length can be smaller)
 */
#define XKL_MAX_CI_SHORT_DESC_LENGTH 10

/*
 * Maximum description length, including '\\0' character
 * (this length is in bytes, so for UTF-8 encoding in 
 * XML file the actual maximum length can be smaller)
 */
#define XKL_MAX_CI_DESC_LENGTH 192

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

	typedef struct _XklConfigItem XklConfigItem;
	typedef struct _XklConfigItemClass XklConfigItemClass;

#define XKL_TYPE_CONFIG_ITEM             (xkl_config_item_get_type ())
#define XKL_CONFIG_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItem))
#define XKL_CONFIG_ITEM_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_ITEM,  XklConfigItemClass))
#define XKL_IS_CONFIG_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_IS_CONFIG_ITEM_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_ITEM))
#define XKL_CONFIG_ITEM_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItemClass))

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
 * Extra property for the XklConfigItem, defining whether the group allows multiple selection
 */
#define XCI_PROP_ALLOW_MULTIPLE_SELECTION "allowMultipleSelection"

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
 * xkl_config_item_get_type:
 * 
 * Get type info for XklConfigItem
 *
 * Returns: GType for XklConfigItem
 */
	extern GType xkl_config_item_get_type(void);

/**
 * xkl_config_item_new:
 *
 * Create new XklConfigItem
 *
 * Returns: new instance
 */
	extern XklConfigItem *xkl_config_item_new(void);

/**
 * ConfigItemProcessFunc:
 * @item: the item from registry
 * @data: anything which can be stored into the pointer
 *
 * Callback type used for enumerating keyboard models, layouts, variants, options
 */
	typedef void (*ConfigItemProcessFunc) (const XklConfigItem * item,
					       gpointer data);

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
