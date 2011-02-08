/*
 * Copyright (C) 2002-2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

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
#define XKL_CONFIG_ITEM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_ITEM, XklConfigItemClass))

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
 * Extra property for the XklConfigItem, defining the vendor (used for models)
 */
#define XCI_PROP_VENDOR "vendor"

/**
 * Extra property for the XklConfigItem, defining the list of countries (used for layouts/variants)
 */
#define XCI_PROP_COUNTRY_LIST "countryList"

/**
 * Extra property for the XklConfigItem, defining the list of languages (used for layouts/variants)
 */
#define XCI_PROP_LANGUAGE_LIST "languageList"

/**
 * Extra property for the XklConfigItem, defining whether that item is exotic(extra)
 */
#define XCI_PROP_EXTRA_ITEM "extraItem"

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

	extern const gchar * xkl_get_country_name(const gchar * code);

	extern const gchar * xkl_get_language_name(const gchar * code);

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
