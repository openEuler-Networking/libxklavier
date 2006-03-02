/**
 * @file xkl_config_registry.h
 */

#ifndef __XKL_CONFIG_REGISTRY_H__
#define __XKL_CONFIG_REGISTRY_H__

#include <glib-object.h>
#include <libxklavier/xkl_engine.h>
#include <libxklavier/xkl_config_item.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef struct _XklConfigRegistry XklConfigRegistry;
	typedef struct _XklConfigRegistryPrivate XklConfigRegistryPrivate;
	typedef struct _XklConfigRegistryClass XklConfigRegistryClass;

#define XKL_TYPE_CONFIG_REGISTRY             (xkl_config_registry_get_type ())
#define XKL_CONFIG_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XKL_TYPE_CONFIG_REGISTRY, XklConfigRegistry))
#define XKL_CONFIG_REGISTRY_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XKL_CONFIG_REGISTRY,  XklConfigRegistryClass))
#define XKL_IS_CONFIG_REGISTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XKL_TYPE_CONFIG_REGISTRY))
#define XKL_IS_CONFIG_REGISTRY_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XKL_TYPE_CONFIG_REGISTRY))
#define XKL_CONFIG_REGISTRY_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XKL_TYPE_CONFIG_REGISTRY, XklConfigRegistryClass))

#endif				// DOXYGEN_SHOULD_SKIP_THIS

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
 * Get type info for XConfig
 * @return GType for XConfig
 */
	extern GType xkl_config_registry_get_type(void);

/**
 * Create new XklConfig
 * @return new instance
 */
	extern XklConfigRegistry
	    * xkl_config_registry_get_instance(XklEngine * engine);

/**
 * @defgroup xklconfiginitterm XKB configuration handling initialization and termination
 * @{
 */

/**
 * Loads XML configuration registry
 * @param file_name file name to load
 * @return true on success
 */
	extern gboolean
	    xkl_config_registry_load_from_file(XklConfigRegistry * config,
					       const gchar * file_name);

/**
 * Loads XML configuration registry
 * @return true on success
 */
	extern gboolean xkl_config_registry_load(XklConfigRegistry *
						 config);

/**
 * Frees XML configuration registry
 */
	extern void xkl_config_registry_free(XklConfigRegistry * config);
/** @} */

/**
 * @defgroup enum XKB configuration elements enumeration functions
 * @{
 */

/**
 * Enumerates keyboard models from the XML configuration registry
 * @param func is a callback to call for every model
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_registry_enum_models(XklConfigRegistry *
						    config,
						    ConfigItemProcessFunc
						    func, gpointer data);

/**
 * Enumerates keyboard layouts from the XML configuration registry
 * @param func is a callback to call for every layout
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_registry_enum_layouts(XklConfigRegistry *
						     config,
						     ConfigItemProcessFunc
						     func, gpointer data);

/**
 * Enumerates keyboard layout variants from the XML configuration registry
 * @param layout_name is the layout name for which variants will be listed
 * @param func is a callback to call for every layout variant
 * @param data is anything which can be stored into the pointer
 */
	extern void
	 xkl_config_registry_enum_layout_variants(XklConfigRegistry *
						  config,
						  const gchar *
						  layout_name,
						  ConfigItemProcessFunc
						  func, gpointer data);

/**
 * Enumerates keyboard option groups from the XML configuration registry
 * @param func is a callback to call for every option group
 * @param data is anything which can be stored into the pointer
 */
	extern void
	 xkl_config_registry_enum_option_groups(XklConfigRegistry *
						config,
						GroupProcessFunc func,
						gpointer data);

/**
 * Enumerates keyboard options from the XML configuration registry
 * @param option_group_name is the option group name for which variants 
 * will be listed
 * @param func is a callback to call for every option
 * @param data is anything which can be stored into the pointer
 */
	extern void xkl_config_registry_enum_options(XklConfigRegistry *
						     config,
						     const gchar *
						     option_group_name,
						     ConfigItemProcessFunc
						     func, gpointer data);

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
	extern gboolean xkl_config_registry_find_model(XklConfigRegistry *
						       config,
						       XklConfigItem *
						       item);

/**
 * Loads a keyboard layout information from the XML configuration registry.
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard layout. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_layout(XklConfigRegistry *
							config,
							XklConfigItem *
							item);

/**
 * Loads a keyboard layout variant information from the XML configuration 
 * registry.
 * @param layout_name is a name of the parent layout
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard layout variant. On successfull return, the descriptions are filled.
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean xkl_config_registry_find_variant(XklConfigRegistry
							 * config,
							 const char
							 *layout_name,
							 XklConfigItem *
							 item);

/**
 * Loads a keyboard option group information from the XML configuration 
 * registry.
 * @param item is a pointer to a XklConfigItem containing the name of the
 * keyboard option group. On successfull return, the descriptions are filled.
 * @param allow_multiple_selection is a pointer to some gboolean variable to fill 
 * the corresponding attribute of XML element "group".
 * @return TRUE if appropriate element was found and loaded
 */
	extern gboolean
	    xkl_config_registry_find_option_group(XklConfigRegistry *
						  config,
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
	extern gboolean xkl_config_registry_find_option(XklConfigRegistry *
							config,
							const gchar *
							option_group_name,
							XklConfigItem *
							item);
/** @} */

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
