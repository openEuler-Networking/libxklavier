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

#define ISO_CODES_DATADIR    ISO_CODES_PREFIX "/share/xml/iso-codes"
#define ISO_CODES_LOCALESDIR ISO_CODES_PREFIX "/share/locale"

static GHashTable *country_code_names = NULL;
static GHashTable *lang_code_names = NULL;

typedef struct {
	const gchar *domain;
	const gchar *attr_names[];
} LookupParams;

typedef struct {
	GHashTable *code_names;
	const gchar *tag_name;
	LookupParams *params;
} CodeBuildStruct;

static LookupParams countryLookup = { "iso_3166", {"alpha_2_code", NULL} };
static LookupParams languageLookup =
    { "iso_639", {"iso_639_2B_code", "iso_639_2T_code", NULL} };

static void
iso_codes_parse_start_tag(GMarkupParseContext * ctx,
			  const gchar * element_name,
			  const gchar ** attr_names,
			  const gchar ** attr_values,
			  gpointer user_data, GError ** error)
{
	const gchar *name;
	const gchar **san = attr_names, **sav = attr_values;
	CodeBuildStruct *cbs = (CodeBuildStruct *) user_data;

	/* Is this the tag we are looking for? */
	if (!g_str_equal(element_name, cbs->tag_name) ||
	    attr_names == NULL || attr_values == NULL) {
		return;
	}

	name = NULL;

	/* What would be the value? */
	while (*attr_names && *attr_values) {
		if (g_str_equal(*attr_names, "name")) {
			name = *attr_values;
			break;
		}

		attr_names++;
		attr_values++;
	}

	if (!name) {
		return;
	}

	attr_names = san;
	attr_values = sav;

	/* Walk again the attributes */
	while (*attr_names && *attr_values) {
		const gchar **attr = cbs->params->attr_names;
		/* Look through all the attributess we are interested in */
		while (*attr) {
			if (g_str_equal(*attr_names, *attr)) {
				if (**attr_values) {
					g_hash_table_insert(cbs->
							    code_names,
							    g_strdup
							    (*attr_values),
							    g_strdup
							    (name));
				}
			}
			attr++;
		}

		attr_names++;
		attr_values++;
	}
}

static GHashTable *
iso_code_names_init(LookupParams * params)
{
	GError *err = NULL;
	gchar *buf, *filename, *tag_name;
	gsize buf_len;
	CodeBuildStruct cbs;

	GHashTable *ht = g_hash_table_new_full(g_str_hash, g_str_equal,
					       g_free, g_free);

	tag_name = g_strdup_printf("%s_entry", params->domain);

	cbs.code_names = ht;
	cbs.tag_name = tag_name;
	cbs.params = params;

	bindtextdomain(params->domain, ISO_CODES_LOCALESDIR);
	bind_textdomain_codeset(params->domain, "UTF-8");

	filename =
	    g_strdup_printf("%s/%s.xml", ISO_CODES_DATADIR,
			    params->domain);
	if (g_file_get_contents(filename, &buf, &buf_len, &err)) {
		GMarkupParseContext *ctx;
		GMarkupParser parser = {
			iso_codes_parse_start_tag,
			NULL, NULL, NULL, NULL
		};

		ctx = g_markup_parse_context_new(&parser, 0, &cbs, NULL);
		if (!g_markup_parse_context_parse(ctx, buf, buf_len, &err)) {
			g_warning("Failed to parse '%s/%s.xml': %s",
				  ISO_CODES_DATADIR,
				  params->domain, err->message);
			g_error_free(err);
		}

		g_markup_parse_context_free(ctx);
		g_free(buf);
	} else {
		g_warning("Failed to load '%s/%s.xml': %s",
			  ISO_CODES_DATADIR, params->domain, err->message);
		g_error_free(err);
	}
	g_free(filename);
	g_free(tag_name);

	return ht;
}

typedef const gchar *(*DescriptionGetterFunc) (const gchar * code);

const gchar *
get_language_iso_code(const gchar * code)
{
	const gchar *name;

	if (!lang_code_names) {
		lang_code_names = iso_code_names_init(&languageLookup);
	}

	name = g_hash_table_lookup(lang_code_names, code);
	if (!name) {
		return NULL;
	}

	return dgettext("iso_639", name);
}

const gchar *
get_country_iso_code(const gchar * code)
{
	const gchar *name;

	if (!country_code_names) {
		country_code_names = iso_code_names_init(&countryLookup);
	}

	name = g_hash_table_lookup(country_code_names, code);
	if (!name) {
		return NULL;
	}

	return dgettext("iso_3166", name);
}

static void
xkl_config_registry_foreach_iso_code(XklConfigRegistry * config,
				     ConfigItemProcessFunc func,
				     const gchar * xpath_exprs[],
				     DescriptionGetterFunc dgf,
				     gboolean to_upper, gpointer data)
{
	GHashTable *code_pairs;
	GHashTableIter iter;
	xmlXPathObjectPtr xpath_obj;
	const gchar **xpath_expr;
	gpointer key, value;
	XklConfigItem *ci;

	if (!xkl_config_registry_is_initialized(config))
		return;

	code_pairs = g_hash_table_new(g_str_hash, g_str_equal);

	for (xpath_expr = xpath_exprs; *xpath_expr; xpath_expr++) {
		xpath_obj =
		    xmlXPathEval((unsigned char *) *xpath_expr,
				 xkl_config_registry_priv(config,
							  xpath_context));
		if (xpath_obj != NULL) {
			gint ni;
			xmlNodeSetPtr nodes = xpath_obj->nodesetval;
			if (nodes != NULL) {
				xmlNodePtr *pnode = nodes->nodeTab;
				for (ni = nodes->nodeNr; --ni >= 0;) {
					gchar *iso_code =
					    (gchar *) (*pnode)->
					    children->content;
					const gchar *description;
					iso_code =
					    to_upper ?
					    g_ascii_strup(iso_code,
							  -1) :
					    g_strdup(iso_code);
					description = dgf(iso_code);
/* If there is a mapping to some ISO description - consider it as ISO code (well, it is just an assumption) */
					if (description)
						g_hash_table_insert
						    (code_pairs,
						     g_strdup
						     (iso_code),
						     g_strdup
						     (description));
					g_free(iso_code);
					pnode++;
				}
			}
			xmlXPathFreeObject(xpath_obj);
		}
	}

	g_hash_table_iter_init(&iter, code_pairs);
	ci = xkl_config_item_new();
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		g_snprintf(ci->name, sizeof(ci->name),
			   (const gchar *) key);
		g_snprintf(ci->description, sizeof(ci->description),
			   (const gchar *) value);
		func(config, ci, data);
	}
	g_object_unref(G_OBJECT(ci));
	g_hash_table_unref(code_pairs);
}

void
xkl_config_registry_foreach_country(XklConfigRegistry *
				    config,
				    ConfigItemProcessFunc
				    func, gpointer data)
{
	const gchar *xpath_exprs[] = {
		XKBCR_LAYOUT_PATH "/configItem/countryList/iso3166Id",
		XKBCR_LAYOUT_PATH "/configItem/name",
		NULL
	};

	xkl_config_registry_foreach_iso_code(config, func, xpath_exprs,
					     get_country_iso_code, TRUE,
					     data);
}

void
xkl_config_registry_foreach_language(XklConfigRegistry *
				     config,
				     ConfigItemProcessFunc
				     func, gpointer data)
{
	const gchar *xpath_exprs[] = {
		XKBCR_LAYOUT_PATH "/configItem/languageList/iso639Id",
		NULL
	};

	xkl_config_registry_foreach_iso_code(config, func, xpath_exprs,
					     get_language_iso_code, FALSE,
					     data);
}

void
xkl_config_registry_foreach_country_variant(XklConfigRegistry *
					    config,
					    const gchar *
					    country_code,
					    TwoConfigItemsProcessFunc
					    func, gpointer data)
{
	const gchar *xpath_exprs[] = {
		XKBCR_LAYOUT_PATH "[configItem/name = '%s']",
		XKBCR_LAYOUT_PATH
		    "[configItem/countryList/iso3166Id = '%s']",
		NULL
	};
	const gboolean are_low_ids[] = { TRUE, FALSE };


	xmlXPathObjectPtr xpath_obj;
	const gchar **xpath_expr;
	const gboolean *is_low_id = are_low_ids;

	if (!xkl_config_registry_is_initialized(config))
		return;

	for (xpath_expr = xpath_exprs; *xpath_expr;
	     xpath_expr++, is_low_id++) {
		gchar *acc =
		    *is_low_id ? g_ascii_strdown(country_code,
						 -1) : g_strdup(country_code);
		gchar *xpe = g_strdup_printf(*xpath_expr, acc);
		g_free(acc);
		xpath_obj =
		    xmlXPathEval((unsigned char *) xpe,
				 xkl_config_registry_priv(config,
							  xpath_context));
		if (xpath_obj != NULL) {
			xmlNodeSetPtr nodes = xpath_obj->nodesetval;
			if (nodes != NULL) {
				gint ni;
				xmlNodePtr *pnode = nodes->nodeTab;
				XklConfigItem *ci = xkl_config_item_new();
				for (ni = nodes->nodeNr; --ni >= 0;) {
					if (xkl_read_config_item
					    (config, *pnode, ci))
						func(config, ci, NULL,
						     data);
					pnode++;
				}
				g_object_unref(G_OBJECT(ci));
			}
			xmlXPathFreeObject(xpath_obj);
		}
		g_free(xpe);
	}
}

void
xkl_config_registry_foreach_language_variant(XklConfigRegistry *
					     config,
					     const gchar *
					     language_code,
					     TwoConfigItemsProcessFunc
					     func, gpointer data)
{
}
