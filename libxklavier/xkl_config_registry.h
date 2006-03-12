#ifndef __XKL_CONFIG_REGISTRY_H__
#define __XKL_CONFIG_REGISTRY_H__

#include <glib-object.h>
#include <libxklavier/xkl_engine.h>
#include <libxklavier/xkl_config_item.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

	typedef struct _XklConfigRegistry XklConfigRegistry;
	typedef struct _XklConfigRegistryPrivate XklConfigRegistryPrivate;
	typedef struct _XklConfigRegistryClass XklConfigRegistryClass;

#define XKL_TYPE_CONFIG_REGISTRY             (xkl_config_registry_get_type ())
#define XKL_CONFIG_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_REGISTRY, XklConfigRegistry))
#define XKL_CONFIG_REGISTRY_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_TYPE_CONFIG_REGISTRY,  XklConfigRegistryClass))
#define XKL_IS_CONFIG_REGISTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_REGISTRY))
#define XKL_IS_CONFIG_REGISTRY_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_REGISTRY))
#define XKL_CONFIG_REGISTRY_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_REGISTRY, XklConfigRegistryClass))

/**
 * The configuration manager. Corresponds to XML element "configItem".
 */
	struct _XklConfigRegistry {
/**
 * The superclass object
 */
		GObject parent;

		XklConfigRegistryPrivate *priv;
	};

/**
 * The XklConfigRegistry class, derived from GObject
 */
	struct _XklConfigRegistryClass {
    /**
     * The superclass
     */
		GObjectClass parent_class;
	};
/**
 * xkl_config_registry_get_type:
 *
 * Get type info for XConfig
 *
 * Returns: GType for XConfig
 */
	extern GType xkl_config_registry_get_type(void);

/**
 * xkl_config_registry_get_instance:
 * @engine: the engine to use for accessing X in all the operations
 * (like accessing root window properties etc)
 *
 * Create new XklConfig
 *
 * Returns: new instance
 */
	extern XklConfigRegistry
	    * xkl_config_registry_get_instance(XklEngine * engine);

/**
 * xkl_config_registry_load:
 * @config: the config registry
 *
 * Loads XML configuration registry. The name is taken from X server
 * (for XKB/libxkbfile, from the root window property)
 *
 * Returns: TRUE on success
 */
	extern gboolean xkl_config_registry_load(XklConfigRegistry *
						 config);

/**
 * xkl_config_registry_foreach_model:
 * @config: the config registry
 * @func: callback to call for every model
 * @data: anything which can be stored into the pointer
 *
 * Enumerates keyboard models from the XML configuration registry
 */
	extern void xkl_config_registry_foreach_model(XklConfigRegistry *
						      config,
						      ConfigItemProcessFunc
						      func, gpointer data);

/**
 * xkl_config_registry_foreach_layout:
 * @config: the config registry
 * @func: callback to call for every layout
 * @data: anything which can be stored into the pointer
 *
 * Enumerates keyboard layouts from the XML configuration registry
 */
	extern void xkl_config_registry_foreach_layout(XklConfigRegistry *
						       config,
						       ConfigItemProcessFunc
						       func,
						       gpointer data);

/**
 * xkl_config_registry_foreach_layout_variant:
 * @config: the config registry
 * @layout_name: layout name for which variants will be listed
 * @func: callback to call for every layout variant
 * @data: anything which can be stored into the pointer
 *
 * Enumerates keyboard layout variants from the XML configuration registry
 */
	extern void
	 xkl_config_registry_foreach_layout_variant(XklConfigRegistry *
						    config,
						    const gchar *
						    layout_name,
						    ConfigItemProcessFunc
						    func, gpointer data);

/**
 * xkl_config_registry_foreach_option_group:
 * @config: the config registry
 * @func: callback to call for every option group
 * @data: anything which can be stored into the pointer
 *
 * Enumerates keyboard option groups from the XML configuration registry
 */
	extern void
	 xkl_config_registry_foreach_option_group(XklConfigRegistry *
						  config,
						  ConfigItemProcessFunc
						  func, gpointer data);

/**
 * xkl_config_registry_foreach_option:
 * @config: the config registry
 * @option_group_name: option group name for which variants 
 * will be listed
 * @func: callback to call for every option
 * @data: anything which can be stored into the pointer
 *
 * Enumerates keyboard options from the XML configuration registry
 */
	extern void xkl_config_registry_foreach_option(XklConfigRegistry *
						       config,
						       const gchar *
						       option_group_name,
						       ConfigItemProcessFunc
						       func,
						       gpointer data);

/**
 * xkl_config_registry_find_model:
 * @config: the config registry
 * @item: pointer to a XklConfigItem containing the name of the
 * keyboard model. On successfull return, the descriptions are filled.
 *
 * Loads a keyboard model information from the XML configuration registry.
 *
 * Returns: TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_model(XklConfigRegistry *
						       config,
						       XklConfigItem *
						       item);

/**
 * xkl_config_registry_find_layout:
 * @config: the config registry
 * @item: pointer to a XklConfigItem containing the name of the
 * keyboard layout. On successfull return, the descriptions are filled.
 *
 * Loads a keyboard layout information from the XML configuration registry.
 *
 * Returns: TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_layout(XklConfigRegistry *
							config,
							XklConfigItem *
							item);

/**
 * xkl_config_registry_find_variant:
 * @config: the config registry
 * @layout_name: name of the parent layout
 * @item: pointer to a XklConfigItem containing the name of the
 * keyboard layout variant. On successfull return, the descriptions are filled.
 *
 * Loads a keyboard layout variant information from the XML configuration 
 * registry.
 *
 * Returns: TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_variant(XklConfigRegistry
							 * config,
							 const char
							 *layout_name,
							 XklConfigItem *
							 item);

/**
 * xkl_config_registry_find_option_group:
 * @config: the config registry
 * @item: pointer to a XklConfigItem containing the name of the
 * keyboard option group. On successfull return, the descriptions are filled.
 *
 * Loads a keyboard option group information from the XML configuration 
 * registry.
 *
 * Returns: TRUE if appropriate element was found and loaded
 */
	extern gboolean
	    xkl_config_registry_find_option_group(XklConfigRegistry *
						  config,
						  XklConfigItem * item);

/**
 * xkl_config_registry_find_option:
 * @config: the config registry
 * @option_group_name: name of the option group
 * @item: pointer to a XklConfigItem containing the name of the
 * keyboard option. On successfull return, the descriptions are filled.
 *
 * Loads a keyboard option information from the XML configuration 
 * registry.
 *
 * Returns: TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_option(XklConfigRegistry *
							config,
							const gchar *
							option_group_name,
							XklConfigItem *
							item);
#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
