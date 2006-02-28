#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <sys/stat.h>

#include "config.h"

#include "xklavier_private.h"

static xmlXPathCompExprPtr models_xpath;
static xmlXPathCompExprPtr layouts_xpath;
static xmlXPathCompExprPtr option_groups_xpath;

#define xkl_config_registry_is_initialized(config) \
  ( (config)->priv->xpath_context != NULL )

static xmlChar *
xkl_node_get_xml_lang_attr(xmlNodePtr nptr)
{
	if (nptr->properties != NULL &&
	    !g_ascii_strcasecmp("lang", (char *) nptr->properties[0].name)
	    && nptr->properties[0].ns != NULL
	    && !g_ascii_strcasecmp("xml",
				   (char *) nptr->properties[0].ns->prefix)
	    && nptr->properties[0].children != NULL)
		return nptr->properties[0].children->content;
	else
		return NULL;
}

static gboolean
xkl_read_config_item(xmlNodePtr iptr, XklConfigItem * item)
{
	xmlNodePtr name_element, nptr, ptr;
	xmlNodePtr desc_element = NULL, short_desc_element = NULL;
	xmlNodePtr nt_desc_element = NULL, nt_short_desc_element = NULL;

	gint max_desc_priority = -1;
	gint max_short_desc_priority = -1;

	*item->name = 0;
	*item->short_description = 0;
	*item->description = 0;
	if (iptr->type != XML_ELEMENT_NODE)
		return FALSE;
	ptr = iptr->children;
	while (ptr != NULL) {
		switch (ptr->type) {
		case XML_ELEMENT_NODE:
			if (!g_ascii_strcasecmp((char *) ptr->name,
						"configItem"))
				break;
			return FALSE;
		case XML_TEXT_NODE:
			ptr = ptr->next;
			continue;
		default:
			return FALSE;
		}
		break;
	}
	if (ptr == NULL)
		return FALSE;

	nptr = ptr->children;

	if (nptr->type == XML_TEXT_NODE)
		nptr = nptr->next;
	name_element = nptr;
	nptr = nptr->next;

	while (nptr != NULL) {
		char *node_name = (char *) nptr->name;
		if (nptr->type != XML_TEXT_NODE) {
			xmlChar *lang = xkl_node_get_xml_lang_attr(nptr);

			if (lang != NULL) {
				gint priority =
				    xkl_get_language_priority((gchar *)
							      lang);

	/**
         * Find desc/shortdesc with highest priority
         */
				if (!g_ascii_strcasecmp(node_name,
							"description") &&
				    (priority > max_desc_priority)) {
					desc_element = nptr;
					max_desc_priority = priority;
				} else if (!g_ascii_strcasecmp(node_name,
							       "shortDescription")
					   && (priority >
					       max_short_desc_priority)) {
					short_desc_element = nptr;
					max_short_desc_priority = priority;
				}
			} else	// no language specified!
			{
				if (!g_ascii_strcasecmp(node_name,
							"description"))
					nt_desc_element = nptr;
				else if (!g_ascii_strcasecmp(node_name,
							     "shortDescription"))
					nt_short_desc_element = nptr;
			}
		}
		nptr = nptr->next;
	}

	/* if no language-specific description found - use the ones without lang */
	if (desc_element == NULL)
		desc_element = nt_desc_element;

	if (short_desc_element == NULL)
		short_desc_element = nt_short_desc_element;

  /**
   * Actually, here we should have some code to find 
   * the correct localized description...
   */

	if (name_element != NULL && name_element->children != NULL)
		strncat(item->name,
			(char *) name_element->children->content,
			XKL_MAX_CI_NAME_LENGTH - 1);

	if (short_desc_element != NULL &&
	    short_desc_element->children != NULL) {
		gchar *lmsg = xkl_locale_from_utf8((const gchar *)
						   short_desc_element->
						   children->content);
		strncat(item->short_description, lmsg,
			XKL_MAX_CI_SHORT_DESC_LENGTH - 1);
		g_free(lmsg);
	}

	if (desc_element != NULL && desc_element->children != NULL) {
		gchar *lmsg =
		    xkl_locale_from_utf8((const gchar *) desc_element->
					 children->content);
		strncat(item->description, lmsg,
			XKL_MAX_CI_DESC_LENGTH - 1);
		g_free(lmsg);
	}
	return TRUE;
}

static void
xkl_config_enum_from_node_set(XklConfig * config, xmlNodeSetPtr nodes,
			      ConfigItemProcessFunc func, gpointer data)
{
	gint i;
	if (nodes != NULL) {
		xmlNodePtr *pnode = nodes->nodeTab;
		for (i = nodes->nodeNr; --i >= 0;) {
			XklConfigItem ci;
			if (xkl_read_config_item(*pnode, &ci))
				func(&ci, data);

			pnode++;
		}
	}
}

static void
xkl_config_enum_simple(XklConfig * config,
		       xmlXPathCompExprPtr xpath_comp_expr,
		       ConfigItemProcessFunc func, gpointer data)
{
	xmlXPathObjectPtr xpath_obj;

	if (!xkl_config_registry_is_initialized(config))
		return;
	xpath_obj = xmlXPathCompiledEval(xpath_comp_expr,
					 config->priv->xpath_context);
	if (xpath_obj != NULL) {
		xkl_config_enum_from_node_set(config,
					      xpath_obj->nodesetval, func,
					      data);
		xmlXPathFreeObject(xpath_obj);
	}
}

static void
xkl_config_enum_direct(XklConfig * config, const gchar * format,
		       const gchar * value,
		       ConfigItemProcessFunc func, gpointer data)
{
	char xpath_expr[1024];
	xmlXPathObjectPtr xpath_obj;

	if (!xkl_config_registry_is_initialized(config))
		return;
	snprintf(xpath_expr, sizeof xpath_expr, format, value);
	xpath_obj = xmlXPathEval((unsigned char *) xpath_expr,
				 config->priv->xpath_context);
	if (xpath_obj != NULL) {
		xkl_config_enum_from_node_set(config,
					      xpath_obj->nodesetval, func,
					      data);
		xmlXPathFreeObject(xpath_obj);
	}
}

static gboolean
xkl_config_find_object(XklConfig * config, const gchar * format,
		       const gchar * arg1,
		       XklConfigItem * pitem /* in/out */ ,
		       xmlNodePtr * pnode /* out */ )
{
	xmlXPathObjectPtr xpath_obj;
	xmlNodeSetPtr nodes;
	gboolean rv = FALSE;
	gchar xpath_expr[1024];

	if (!xkl_config_registry_is_initialized(config))
		return FALSE;

	snprintf(xpath_expr, sizeof xpath_expr, format, arg1, pitem->name);
	xpath_obj = xmlXPathEval((unsigned char *) xpath_expr,
				 config->priv->xpath_context);
	if (xpath_obj == NULL)
		return FALSE;

	nodes = xpath_obj->nodesetval;
	if (nodes != NULL && nodes->nodeTab != NULL) {
		rv = xkl_read_config_item(*nodes->nodeTab, pitem);
		if (pnode != NULL) {
			*pnode = *nodes->nodeTab;
		}
	}

	xmlXPathFreeObject(xpath_obj);
	return rv;
}

gchar *
xkl_config_rec_merge_layouts(const XklConfigRec * data)
{
	return xkl_strings_concat_comma_separated(data->layouts);
}

gchar *
xkl_config_rec_merge_variants(const XklConfigRec * data)
{
	return xkl_strings_concat_comma_separated(data->variants);
}

gchar *
xkl_config_rec_merge_options(const XklConfigRec * data)
{
	return xkl_strings_concat_comma_separated(data->options);
}

gchar *
xkl_strings_concat_comma_separated(gchar ** array)
{
	return g_strjoinv(",", array);
}

void
xkl_config_rec_split_layouts(XklConfigRec * data, const gchar * merged)
{
	xkl_strings_split_comma_separated(&data->layouts, merged);
}

void
xkl_config_rec_split_variants(XklConfigRec * data, const gchar * merged)
{
	xkl_strings_split_comma_separated(&data->variants, merged);
}

void
xkl_config_rec_split_options(XklConfigRec * data, const gchar * merged)
{
	xkl_strings_split_comma_separated(&data->options, merged);
}

void
xkl_strings_split_comma_separated(gchar *** array, const gchar * merged)
{
	*array = g_strsplit(merged, ",", 0);
}

gchar *
xkl_engine_get_ruleset_name(XklEngine * engine,
			    const gchar default_ruleset[])
{
	static gchar rules_set_name[1024] = "";
	if (!rules_set_name[0]) {
		/* first call */
		gchar *rf = NULL;
		if (!xkl_get_names_prop
		    (engine->priv->base_config_atom, &rf, NULL)
		    || (rf == NULL)) {
			g_strlcpy(rules_set_name, default_ruleset,
				  sizeof rules_set_name);
			xkl_debug(100, "Using default rules set: [%s]\n",
				  rules_set_name);
			return rules_set_name;
		}
		g_strlcpy(rules_set_name, rf, sizeof rules_set_name);
		g_free(rf);
	}
	xkl_debug(100, "Rules set: [%s]\n", rules_set_name);
	return rules_set_name;
}

void
xkl_config_init(XklConfig * config)
{
	xmlXPathInit();
	models_xpath = xmlXPathCompile((unsigned char *)
				       "/xkbConfigRegistry/modelList/model");
	layouts_xpath = xmlXPathCompile((unsigned char *)
					"/xkbConfigRegistry/layoutList/layout");
	option_groups_xpath = xmlXPathCompile((unsigned char *)
					      "/xkbConfigRegistry/optionList/group");
	xkl_i18n_init();

	XklEngine *engine = xkl_config_get_engine(config);
	xkl_engine_ensure_vtable_inited(engine);
	xkl_engine_vcall(engine, init_config) (config);
}

void
xkl_config_term(void)
{
	if (models_xpath != NULL) {
		xmlXPathFreeCompExpr(models_xpath);
		models_xpath = NULL;
	}
	if (layouts_xpath != NULL) {
		xmlXPathFreeCompExpr(layouts_xpath);
		layouts_xpath = NULL;
	}
	if (option_groups_xpath != NULL) {
		xmlXPathFreeCompExpr(option_groups_xpath);
		option_groups_xpath = NULL;
	}
}

gboolean
xkl_config_registry_load_from_file(XklConfig * config,
				   const gchar * file_name)
{
	config->priv->doc = xmlParseFile(file_name);
	if (config->priv->doc == NULL) {
		config->priv->xpath_context = NULL;
		xkl_config_get_engine(config)->priv->last_error_message =
		    "Could not parse XKB configuration registry";
	} else
		config->priv->xpath_context =
		    xmlXPathNewContext(config->priv->doc);
	return xkl_config_registry_is_initialized(config);
}

void
xkl_config_registry_free(XklConfig * config)
{
	if (xkl_config_registry_is_initialized(config)) {
		xmlXPathFreeContext(config->priv->xpath_context);
		xmlFreeDoc(config->priv->doc);
		config->priv->xpath_context = NULL;
		config->priv->doc = NULL;
	}
}

void
xkl_config_enum_models(XklConfig * config, ConfigItemProcessFunc func,
		       gpointer data)
{
	xkl_config_enum_simple(config, models_xpath, func, data);
}

void
xkl_config_enum_layouts(XklConfig * config, ConfigItemProcessFunc func,
			gpointer data)
{
	xkl_config_enum_simple(config, layouts_xpath, func, data);
}

void
xkl_config_enum_layout_variants(XklConfig * config,
				const gchar * layout_name,
				ConfigItemProcessFunc func, gpointer data)
{
	xkl_config_enum_direct
	    (config,
	     "/xkbConfigRegistry/layoutList/layout/variantList/variant[../../configItem/name = '%s']",
	     layout_name, func, data);
}

void
xkl_config_enum_option_groups(XklConfig * config, GroupProcessFunc func,
			      gpointer data)
{
	xmlXPathObjectPtr xpath_obj;
	gint i;

	if (!xkl_config_registry_is_initialized(config))
		return;
	xpath_obj =
	    xmlXPathCompiledEval(option_groups_xpath,
				 config->priv->xpath_context);
	if (xpath_obj != NULL) {
		xmlNodeSetPtr nodes = xpath_obj->nodesetval;
		xmlNodePtr *pnode = nodes->nodeTab;
		for (i = nodes->nodeNr; --i >= 0;) {
			XklConfigItem ci;

			if (xkl_read_config_item(*pnode, &ci)) {
				gboolean allow_multisel = TRUE;
				xmlChar *sallow_multisel =
				    xmlGetProp(*pnode,
					       (unsigned char *)
					       "allowMultipleSelection");
				if (sallow_multisel != NULL) {
					allow_multisel =
					    !g_ascii_strcasecmp("true",
								(char *)
								sallow_multisel);
					xmlFree(sallow_multisel);
				}

				func(&ci, allow_multisel, data);
			}

			pnode++;
		}
		xmlXPathFreeObject(xpath_obj);
	}
}

void
xkl_config_enum_options(XklConfig * config,
			const gchar * option_group_name,
			ConfigItemProcessFunc func, gpointer data)
{
	xkl_config_enum_direct
	    (config,
	     "/xkbConfigRegistry/optionList/group/option[../configItem/name = '%s']",
	     option_group_name, func, data);
}

gboolean
xkl_config_find_model(XklConfig * config,
		      XklConfigItem * pitem /* in/out */ )
{
	return
	    xkl_config_find_object
	    (config,
	     "/xkbConfigRegistry/modelList/model[configItem/name = '%s%s']",
	     "", pitem, NULL);
}

gboolean
xkl_config_find_layout(XklConfig * config,
		       XklConfigItem * pitem /* in/out */ )
{
	return
	    xkl_config_find_object
	    (config,
	     "/xkbConfigRegistry/layoutList/layout[configItem/name = '%s%s']",
	     "", pitem, NULL);
}

gboolean
xkl_config_find_variant(XklConfig * config, const char *layout_name,
			XklConfigItem * pitem /* in/out */ )
{
	return
	    xkl_config_find_object
	    (config,
	     "/xkbConfigRegistry/layoutList/layout/variantList/variant"
	     "[../../configItem/name = '%s' and configItem/name = '%s']",
	     layout_name, pitem, NULL);
}

gboolean
_xkl_config_find_option_group(XklConfig * config,
			      XklConfigItem * pitem /* in/out */ ,
			      gboolean *
			      allow_multiple_selection /* out */ )
{
	xmlNodePtr node;
	gboolean rv = xkl_config_find_object(config,
					     "/xkbConfigRegistry/optionList/group[configItem/name = '%s%s']",
					     "",
					     pitem, &node);

	if (rv && allow_multiple_selection != NULL) {
		xmlChar *val = xmlGetProp(node,
					  (unsigned char *)
					  "allowMultipleSelection");
		*allow_multiple_selection = FALSE;
		if (val != NULL) {
			*allow_multiple_selection =
			    !g_ascii_strcasecmp("true", (char *) val);
			xmlFree(val);
		}
	}
	return rv;
}

gboolean
xkl_config_find_option(XklConfig * config, const char *option_group_name,
		       XklConfigItem * pitem /* in/out */ )
{
	return
	    xkl_config_find_object
	    (config, "/xkbConfigRegistry/optionList/group/option"
	     "[../configItem/name = '%s' and configItem/name = '%s']",
	     option_group_name, pitem, NULL);
}

/**
 * Calling through vtable
 */
gboolean
xkl_config_activate(XklConfig * config, const XklConfigRec * data)
{
	XklEngine *engine = xkl_config_get_engine(config);
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, activate_config) (config, data);
}

gboolean
xkl_config_registry_load(XklConfig * config)
{
	XklEngine *engine = xkl_config_get_engine(config);
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, load_config_registry) (config);
}

gboolean
xkl_config_write_file(XklConfig * config, const gchar * file_name,
		      const XklConfigRec * data, const gboolean binary)
{
	XklEngine *engine = xkl_config_get_engine(config);
	if ((!binary &&
	     !(engine->priv->features & XKLF_CAN_OUTPUT_CONFIG_AS_ASCII))
	    || (binary
		&& !(engine->priv->
		     features & XKLF_CAN_OUTPUT_CONFIG_AS_BINARY))) {
		engine->priv->last_error_message =
		    "Function not supported at backend";
		return FALSE;
	}
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, write_config_to_file) (config,
							       file_name,
							       data,
							       binary);
}

void
xkl_config_dump(FILE * file, XklConfigRec * data)
{
	int j;
	fprintf(file, "  model: [%s]\n", data->model);

	fprintf(file, "  layouts:\n");
#define OUTPUT_ARRZ(arrz) \
  { \
    fprintf( file, "  " #arrz ":\n" ); \
    gchar **p = data->arrz; \
    if ( p != NULL ) \
      for( j = 0; *p != NULL; ) \
        fprintf( file, "  %d: [%s]\n", j++, *p++ ); \
  }
	OUTPUT_ARRZ(layouts);
	OUTPUT_ARRZ(variants);
	OUTPUT_ARRZ(options);

}
