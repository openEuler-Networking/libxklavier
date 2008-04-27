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

#include <errno.h>
#include <locale.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "config.h"

#include "xklavier_private.h"

static GObjectClass *parent_class = NULL;

static xmlXPathCompExprPtr models_xpath;
static xmlXPathCompExprPtr layouts_xpath;
static xmlXPathCompExprPtr option_groups_xpath;

/* gettext domain for translations */
#define XKB_DOMAIN "xkeyboard-config"

enum {
	PROP_0,
	PROP_ENGINE,
};

static gboolean
xkl_xml_find_config_item_child(xmlNodePtr iptr, xmlNodePtr * ptr)
{
	/* 
	 * Walking through the 1st level children of iptr
	 * looking for the configItem
	 */
	if (iptr->type != XML_ELEMENT_NODE)
		return FALSE;
	*ptr = iptr->children;
	while (*ptr != NULL) {
		switch ((*ptr)->type) {
		case XML_ELEMENT_NODE:
			return !g_ascii_strcasecmp
			    ((char *) (*ptr)->name, "configItem");
		case XML_TEXT_NODE:
		case XML_COMMENT_NODE:
			(*ptr) = (*ptr)->next;
			continue;
		default:
			return FALSE;
		}
		break;
	}
	return FALSE;
}

static xmlNodePtr
xkl_find_nonlocalized_element(xmlNodePtr ptr, const gchar * tag_name)
{
	xmlNodePtr found_element = NULL;

	/* Look through all siblings, trying to find a node with proper name */
	while (ptr != NULL) {
		char *node_name = (char *) ptr->name;
		if (ptr->type != XML_TEXT_NODE) {
			xmlChar *lang = xmlNodeGetLang(ptr);

			if (lang == NULL) {	/* No language specified */
				if (!g_ascii_strcasecmp
				    (node_name, tag_name)) {
					found_element = ptr;
					break;
				}
			}
		}
		ptr = ptr->next;
	}
	return found_element;
}

static gboolean
xkl_item_populate_optional_array(XklConfigItem * item, xmlNodePtr ptr,
				 const gchar list_tag[],
				 const gchar element_tag[],
				 const gchar property_name[])
{
	xmlNodePtr top_list_element =
	    xkl_find_nonlocalized_element(ptr, list_tag), element_ptr;
	gint n_elements, idx;
	gchar **elements = NULL;

	if (top_list_element == NULL || top_list_element->children == NULL)
		return FALSE;

	n_elements = 0;

	/* First, count countries */
	element_ptr = top_list_element->children;
	while (NULL !=
	       (element_ptr =
		xkl_find_nonlocalized_element(element_ptr, element_tag))) {
		n_elements++;
		element_ptr = element_ptr->next;
	}

	if (n_elements == 0)
		return FALSE;

	elements = g_new0(gchar *, n_elements + 1);
	/* Then, actually, populate the list */
	element_ptr = top_list_element->children;
	for (idx = 0;
	     NULL != (element_ptr =
		      xkl_find_nonlocalized_element
		      (element_ptr, element_tag));
	     element_ptr = element_ptr->next, idx++) {
		elements[idx] =
		    g_strdup((const char *) element_ptr->
			     children->content);
	}

	g_object_set_data_full(G_OBJECT(item),
			       property_name, elements, (GDestroyNotify)
			       g_strfreev);
	return TRUE;
}

gboolean
xkl_read_config_item(XklConfigRegistry * config, xmlNodePtr iptr,
		     XklConfigItem * item)
{
	xmlNodePtr name_element, ptr;
	xmlNodePtr desc_element = NULL, short_desc_element =
	    NULL, vendor_element = NULL;

	gchar *vendor = NULL;

	*item->name = 0;
	*item->short_description = 0;
	*item->description = 0;

	g_object_set_data(G_OBJECT(item), XCI_PROP_VENDOR, NULL);
	g_object_set_data(G_OBJECT(item), XCI_PROP_COUNTRY_LIST, NULL);
	g_object_set_data(G_OBJECT(item), XCI_PROP_LANGUAGE_LIST, NULL);

	if (!xkl_xml_find_config_item_child(iptr, &ptr))
		return FALSE;

	ptr = ptr->children;

	if (ptr->type == XML_TEXT_NODE)
		ptr = ptr->next;
	name_element = ptr;
	ptr = ptr->next;

	short_desc_element =
	    xkl_find_nonlocalized_element(ptr, XML_TAG_SHORT_DESCR);
	desc_element = xkl_find_nonlocalized_element(ptr, XML_TAG_DESCR);
	vendor_element =
	    xkl_find_nonlocalized_element(ptr, XML_TAG_VENDOR);

	if (name_element != NULL && name_element->children != NULL)
		strncat(item->name,
			(char *) name_element->children->content,
			XKL_MAX_CI_NAME_LENGTH - 1);

	if (short_desc_element != NULL
	    && short_desc_element->children != NULL) {
		strncat(item->short_description,
			dgettext(XKB_DOMAIN,
				 (const char *) short_desc_element->
				 children->content),
			XKL_MAX_CI_SHORT_DESC_LENGTH - 1);
	}

	if (desc_element != NULL && desc_element->children != NULL) {
		strncat(item->description,
			dgettext(XKB_DOMAIN,
				 (const char *) desc_element->children->
				 content), XKL_MAX_CI_DESC_LENGTH - 1);
	}

	if (vendor_element != NULL && vendor_element->children != NULL) {
		vendor =
		    g_strdup((const char *) vendor_element->children->
			     content);
		g_object_set_data_full(G_OBJECT(item), XCI_PROP_VENDOR,
				       vendor, g_free);
	}

	xkl_item_populate_optional_array(item, ptr, XML_TAG_COUNTRY_LIST,
					 XML_TAG_ISO3166ID,
					 XCI_PROP_COUNTRY_LIST);
	xkl_item_populate_optional_array(item, ptr, XML_TAG_LANGUAGE_LIST,
					 XML_TAG_ISO639ID,
					 XCI_PROP_LANGUAGE_LIST);

	return TRUE;
}

static void
xkl_config_registry_foreach_in_nodeset(XklConfigRegistry * config,
				       xmlNodeSetPtr nodes,
				       ConfigItemProcessFunc func,
				       gpointer data)
{
	gint i;
	if (nodes != NULL) {
		xmlNodePtr *pnode = nodes->nodeTab;
		XklConfigItem *ci = xkl_config_item_new();
		for (i = nodes->nodeNr; --i >= 0;) {
			if (xkl_read_config_item(config, *pnode, ci))
				func(config, ci, data);

			pnode++;
		}
		g_object_unref(G_OBJECT(ci));
	}
}

void
xkl_config_registry_foreach_in_xpath(XklConfigRegistry * config,
				     xmlXPathCompExprPtr
				     xpath_comp_expr,
				     ConfigItemProcessFunc func,
				     gpointer data)
{
	xmlXPathObjectPtr xpath_obj;

	if (!xkl_config_registry_is_initialized(config))
		return;
	xpath_obj = xmlXPathCompiledEval(xpath_comp_expr,
					 xkl_config_registry_priv
					 (config, xpath_context));
	if (xpath_obj != NULL) {
		xkl_config_registry_foreach_in_nodeset(config,
						       xpath_obj->
						       nodesetval,
						       func, data);
		xmlXPathFreeObject(xpath_obj);
	}
}

void
xkl_config_registry_foreach_in_xpath_with_param(XklConfigRegistry
						* config,
						const gchar *
						format,
						const gchar *
						value,
						ConfigItemProcessFunc
						func, gpointer data)
{
	char xpath_expr[1024];
	xmlXPathObjectPtr xpath_obj;

	if (!xkl_config_registry_is_initialized(config))
		return;
	g_snprintf(xpath_expr, sizeof xpath_expr, format, value);
	xpath_obj = xmlXPathEval((unsigned char *) xpath_expr,
				 xkl_config_registry_priv(config,
							  xpath_context));
	if (xpath_obj != NULL) {
		xkl_config_registry_foreach_in_nodeset(config,
						       xpath_obj->
						       nodesetval,
						       func, data);
		xmlXPathFreeObject(xpath_obj);
	}
}

static gboolean
xkl_config_registry_find_object(XklConfigRegistry * config,
				const gchar * format,
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

	g_snprintf(xpath_expr, sizeof xpath_expr, format, arg1,
		   pitem->name);
	xpath_obj =
	    xmlXPathEval((unsigned char *) xpath_expr,
			 xkl_config_registry_priv(config, xpath_context));
	if (xpath_obj == NULL)
		return FALSE;

	nodes = xpath_obj->nodesetval;
	if (nodes != NULL && nodes->nodeTab != NULL && nodes->nodeNr > 0) {
		rv = xkl_read_config_item(config, nodes->nodeTab[0],
					  pitem);
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
	if (array) {
		return g_strjoinv(",", array);
	} else {
		return g_strdup("");
	}
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
		if (!xkl_config_rec_get_from_root_window_property
		    (NULL,
		     xkl_engine_priv(engine, base_config_atom),
		     &rf, engine) || (rf == NULL)) {
			g_strlcpy(rules_set_name, default_ruleset,
				  sizeof rules_set_name);
			xkl_debug(100,
				  "Using default rules set: [%s]\n",
				  rules_set_name);
			return rules_set_name;
		}
		g_strlcpy(rules_set_name, rf, sizeof rules_set_name);
		g_free(rf);
	}
	xkl_debug(100, "Rules set: [%s]\n", rules_set_name);
	return rules_set_name;
}

XklConfigRegistry *
xkl_config_registry_get_instance(XklEngine * engine)
{
	XklConfigRegistry *config;

	if (!engine) {
		xkl_debug(10,
			  "xkl_config_registry_get_instance : engine is NULL ?\n");
		return NULL;
	}

	config =
	    XKL_CONFIG_REGISTRY(g_object_new
				(xkl_config_registry_get_type(),
				 "engine", engine, NULL));

	return config;
}

/* We process descriptions as "leaf" elements - this is ok for base.xml*/
static gboolean skipping_tag = FALSE;

static void
xkl_xml_sax_start_element_ns(void *ctx,
			     const xmlChar * localname,
			     const xmlChar * prefix,
			     const xmlChar * URI,
			     int nb_namespaces,
			     const xmlChar ** namespaces,
			     int nb_attributes,
			     int nb_defaulted, const xmlChar ** attributes)
{
	int i;
	gchar *lang = NULL;
	for (i = 0; i < nb_attributes * 5; i += 5) {
		int len = attributes[i + 4] - attributes[i + 3];
		gchar *value = g_new0(gchar, len + 1);
		memcpy(value, attributes[i + 3], len);
		if (!g_ascii_strcasecmp((gchar *) attributes[i], "lang")
		    /* && ... */
		    ) {
			lang = value;
			break;
		}
		g_free(value);
	}
	if (lang != NULL) {
		g_free(lang);
		skipping_tag = TRUE;
		return;
	}
	xmlSAX2StartElementNs(ctx, localname, prefix, URI,
			      nb_namespaces, namespaces, nb_attributes,
			      nb_defaulted, attributes);
}

static void
xkl_xml_sax_characters(void *ctx, const xmlChar * ch, int len)
{
	if (!skipping_tag) {
		xmlSAX2Characters(ctx, ch, len);
	}
}

static void
xkl_xml_sax_end_element_ns(void *ctx,
			   const xmlChar * localname,
			   const xmlChar * prefix, const xmlChar * URI)
{
	if (skipping_tag) {
		skipping_tag = FALSE;
	} else {
		xmlSAX2EndElementNs(ctx, localname, prefix, URI);
	}
}

gboolean
xkl_config_registry_load_from_file(XklConfigRegistry * config,
				   const gchar * file_name)
{
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	xmlSAXHandler *saxh = g_new0(xmlSAXHandler, 1);
	xkl_debug(100, "Loading XML registry from file %s\n", file_name);

	/* Filter out all unneeded languages! */
	xmlSAX2InitDefaultSAXHandler(saxh, TRUE);
	saxh->startElementNs = xkl_xml_sax_start_element_ns;
	saxh->endElementNs = xkl_xml_sax_end_element_ns;
	saxh->characters = xkl_xml_sax_characters;
	ctxt->sax = saxh;

	xkl_config_registry_priv(config, doc) =
	    xmlCtxtReadFile(ctxt, file_name, NULL, XML_PARSE_NOBLANKS);
	xmlFreeParserCtxt(ctxt);

	if (xkl_config_registry_priv(config, doc) == NULL) {
		xkl_config_registry_priv(config, xpath_context) = NULL;
		xkl_last_error_message =
		    "Could not parse XKB configuration registry";
		return FALSE;
	}
	xkl_config_registry_priv(config, xpath_context) =
	    xmlXPathNewContext(xkl_config_registry_priv(config, doc));

	return TRUE;
}

gboolean
xkl_config_registry_load_helper(XklConfigRegistry * config,
				const char default_ruleset[],
				const char base_dir[])
{
	struct stat stat_buf;
	gchar file_name[MAXPATHLEN] = "";
	XklEngine *engine = xkl_config_registry_get_engine(config);
	gchar *rf = xkl_engine_get_ruleset_name(engine, default_ruleset);

	if (rf == NULL || rf[0] == '\0')
		return FALSE;

	g_snprintf(file_name, sizeof file_name, "%s/%s.xml", base_dir, rf);

	if (stat(file_name, &stat_buf) != 0) {
		xkl_debug(0, "Missing registry file %s\n", file_name);
		xkl_last_error_message = "Missing registry file";
		return FALSE;
	}

	return xkl_config_registry_load_from_file(config, file_name);
}

void
xkl_config_registry_free(XklConfigRegistry * config)
{
	if (xkl_config_registry_is_initialized(config)) {
		xmlXPathFreeContext(xkl_config_registry_priv
				    (config, xpath_context));
		xmlFreeDoc(xkl_config_registry_priv(config, doc));
		xkl_config_registry_priv(config, xpath_context) = NULL;
		xkl_config_registry_priv(config, doc) = NULL;
	}
}

void
xkl_config_registry_foreach_model(XklConfigRegistry * config,
				  ConfigItemProcessFunc func,
				  gpointer data)
{
	xkl_config_registry_foreach_in_xpath(config, models_xpath,
					     func, data);
}

void
xkl_config_registry_foreach_layout(XklConfigRegistry * config,
				   ConfigItemProcessFunc func,
				   gpointer data)
{
	xkl_config_registry_foreach_in_xpath(config, layouts_xpath,
					     func, data);
}

void
xkl_config_registry_foreach_layout_variant(XklConfigRegistry *
					   config,
					   const gchar *
					   layout_name,
					   ConfigItemProcessFunc
					   func, gpointer data)
{
	xkl_config_registry_foreach_in_xpath_with_param(config,
							XKBCR_VARIANT_PATH
							"[../../configItem/name = '%s']",
							layout_name,
							func, data);
}

void
xkl_config_registry_foreach_option_group(XklConfigRegistry *
					 config,
					 ConfigItemProcessFunc
					 func, gpointer data)
{
	xmlXPathObjectPtr xpath_obj;
	gint i;

	if (!xkl_config_registry_is_initialized(config))
		return;
	xpath_obj =
	    xmlXPathCompiledEval(option_groups_xpath,
				 xkl_config_registry_priv(config,
							  xpath_context));
	if (xpath_obj != NULL) {
		xmlNodeSetPtr nodes = xpath_obj->nodesetval;
		xmlNodePtr *pnode = nodes->nodeTab;
		XklConfigItem *ci = xkl_config_item_new();
		for (i = nodes->nodeNr; --i >= 0;) {

			if (xkl_read_config_item(config, *pnode, ci)) {
				gboolean allow_multisel = TRUE;
				xmlChar *sallow_multisel =
				    xmlGetProp(*pnode,
					       (unsigned char *)
					       XCI_PROP_ALLOW_MULTIPLE_SELECTION);
				if (sallow_multisel != NULL) {
					allow_multisel =
					    !g_ascii_strcasecmp
					    ("true", (char *)
					     sallow_multisel);
					xmlFree(sallow_multisel);
					g_object_set_data(G_OBJECT
							  (ci),
							  XCI_PROP_ALLOW_MULTIPLE_SELECTION,
							  GINT_TO_POINTER
							  (allow_multisel));
				}

				func(config, ci, data);
			}

			pnode++;
		}
		g_object_unref(G_OBJECT(ci));
		xmlXPathFreeObject(xpath_obj);
	}
}

void
xkl_config_registry_foreach_option(XklConfigRegistry * config,
				   const gchar *
				   option_group_name,
				   ConfigItemProcessFunc func,
				   gpointer data)
{
	xkl_config_registry_foreach_in_xpath_with_param(config,
							XKBCR_OPTION_PATH
							"[../configItem/name = '%s']",
							option_group_name,
							func, data);
}

gboolean
xkl_config_registry_find_model(XklConfigRegistry * config,
			       XklConfigItem * pitem /* in/out */ )
{
	return xkl_config_registry_find_object(config,
					       XKBCR_MODEL_PATH
					       "[configItem/name = '%s%s']",
					       "", pitem, NULL);
}

gboolean
xkl_config_registry_find_layout(XklConfigRegistry * config,
				XklConfigItem * pitem /* in/out */ )
{
	return xkl_config_registry_find_object(config,
					       XKBCR_LAYOUT_PATH
					       "[configItem/name = '%s%s']",
					       "", pitem, NULL);
}

gboolean
xkl_config_registry_find_variant(XklConfigRegistry * config,
				 const char *layout_name,
				 XklConfigItem * pitem /* in/out */ )
{
	return xkl_config_registry_find_object(config,
					       XKBCR_VARIANT_PATH
					       "[../../configItem/name = '%s' and configItem/name = '%s']",
					       layout_name, pitem, NULL);
}

gboolean
xkl_config_registry_find_option_group(XklConfigRegistry *
				      config,
				      XklConfigItem * pitem /* in/out */ )
{
	xmlNodePtr node = NULL;
	gboolean rv = xkl_config_registry_find_object(config,
						      XKBCR_GROUP_PATH
						      "[configItem/name = '%s%s']",
						      "",
						      pitem,
						      &node);

	if (rv) {
		xmlChar *val = xmlGetProp(node,
					  (unsigned char *)
					  XCI_PROP_ALLOW_MULTIPLE_SELECTION);
		if (val != NULL) {
			gboolean allow_multisel =
			    !g_ascii_strcasecmp("true",
						(char *) val);
			g_object_set_data(G_OBJECT(pitem),
					  XCI_PROP_ALLOW_MULTIPLE_SELECTION,
					  GINT_TO_POINTER(allow_multisel));
			xmlFree(val);
		}
	}
	return rv;
}

gboolean
xkl_config_registry_find_option(XklConfigRegistry * config,
				const char *option_group_name,
				XklConfigItem * pitem /* in/out */ )
{
	return xkl_config_registry_find_object(config,
					       XKBCR_OPTION_PATH
					       "[../configItem/name = '%s' and configItem/name = '%s']",
					       option_group_name,
					       pitem, NULL);
}

/*
 * Calling through vtable
 */
gboolean
xkl_config_rec_activate(const XklConfigRec * data, XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine,
				activate_config_rec) (engine, data);
}

gboolean
xkl_config_registry_load(XklConfigRegistry * config)
{
	XklEngine *engine;

	xkl_config_registry_free(config);
	engine = xkl_config_registry_get_engine(config);
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, load_config_registry) (config);
}

gboolean
xkl_config_rec_write_to_file(XklEngine * engine,
			     const gchar * file_name,
			     const XklConfigRec * data,
			     const gboolean binary)
{
	if ((!binary
	     && !(xkl_engine_priv(engine, features) &
		  XKLF_CAN_OUTPUT_CONFIG_AS_ASCII))
	    || (binary
		&& !(xkl_engine_priv(engine, features) &
		     XKLF_CAN_OUTPUT_CONFIG_AS_BINARY))) {
		xkl_last_error_message =
		    "Function not supported at backend";
		return FALSE;
	}
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine,
				write_config_rec_to_file) (engine,
							   file_name,
							   data, binary);
}

void
xkl_config_rec_dump(FILE * file, XklConfigRec * data)
{
	int j;
	fprintf(file, "  model: [%s]\n", data->model);

	fprintf(file, "  layouts:\n");
#define OUTPUT_ARRZ(arrz) \
  { \
    gchar **p = data->arrz; \
    fprintf( file, "  " #arrz ":\n" ); \
    if ( p != NULL ) \
      for( j = 0; *p != NULL; ) \
        fprintf( file, "  %d: [%s]\n", j++, *p++ ); \
  }
	OUTPUT_ARRZ(layouts);
	OUTPUT_ARRZ(variants);
	OUTPUT_ARRZ(options);

}

G_DEFINE_TYPE(XklConfigRegistry, xkl_config_registry, G_TYPE_OBJECT)
static GObject *
xkl_config_registry_constructor(GType type,
				guint
				n_construct_properties,
				GObjectConstructParam
				* construct_properties)
{
	GObject *obj;
	XklConfigRegistry *config;
	XklEngine *engine;

	{
		/* Invoke parent constructor. */
		XklConfigRegistryClass *klass;
		klass =
		    XKL_CONFIG_REGISTRY_CLASS(g_type_class_peek
					      (XKL_TYPE_CONFIG_REGISTRY));
		obj =
		    parent_class->constructor(type,
					      n_construct_properties,
					      construct_properties);
	}

	config = XKL_CONFIG_REGISTRY(obj);

	engine = XKL_ENGINE(g_value_peek_pointer
			    (construct_properties[0].value));
	xkl_config_registry_get_engine(config) = engine;

	xkl_engine_ensure_vtable_inited(engine);
	xkl_engine_vcall(engine, init_config_registry) (config);

	return obj;
}

static void
xkl_config_registry_init(XklConfigRegistry * config)
{
	config->priv = g_new0(XklConfigRegistryPrivate, 1);
}

static void
xkl_config_registry_set_property(GObject * object,
				 guint property_id,
				 const GValue * value, GParamSpec * pspec)
{
}

static void
xkl_config_registry_get_property(GObject * object,
				 guint property_id,
				 GValue * value, GParamSpec * pspec)
{
	XklConfigRegistry *config = XKL_CONFIG_REGISTRY(object);

	switch (property_id) {
	case PROP_ENGINE:
		g_value_set_pointer(value,
				    xkl_config_registry_get_engine
				    (config));
		break;
	}

}

static void
xkl_config_registry_finalize(GObject * obj)
{
	XklConfigRegistry *config = (XklConfigRegistry *) obj;

	xkl_config_registry_free(config);

	g_free(config->priv);

	G_OBJECT_CLASS(parent_class)->finalize(obj);
}

/* 
 * This function is actually NEVER called.
 * It is 'extern' just to avoid the compilation warnings
 * TODO: add class cleanup
 */
extern void
xkl_config_registry_class_term(XklConfigRegistryClass * klass)
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

static void
xkl_config_registry_class_init(XklConfigRegistryClass * klass)
{
	GObjectClass *object_class;
	GParamSpec *engine_param_spec;

	object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent(object_class);
	object_class->constructor = xkl_config_registry_constructor;
	object_class->finalize = xkl_config_registry_finalize;
	object_class->set_property = xkl_config_registry_set_property;
	object_class->get_property = xkl_config_registry_get_property;

	engine_param_spec = g_param_spec_object("engine",
						"Engine",
						"XklEngine",
						XKL_TYPE_ENGINE,
						G_PARAM_CONSTRUCT_ONLY
						| G_PARAM_READWRITE);

	g_object_class_install_property(object_class,
					PROP_ENGINE, engine_param_spec);

	/* static stuff initialized */

	xmlXPathInit();
	models_xpath = xmlXPathCompile((unsigned char *)
				       XKBCR_MODEL_PATH);
	layouts_xpath = xmlXPathCompile((unsigned char *)
					XKBCR_LAYOUT_PATH);
	option_groups_xpath = xmlXPathCompile((unsigned char *)
					      XKBCR_GROUP_PATH);
}
