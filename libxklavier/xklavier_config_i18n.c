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

#include <iconv.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

#ifdef HAVE_SETLOCALE
# include <locale.h>
#endif

#include "xklavier_private.h"

#define MAX_LOCALE_LEN 128

static gchar locale_substrings[3][MAX_LOCALE_LEN];

/*
 *  some bad guys create LC_ALL=LC_CTYPE=ru_RU.UTF-8;LC_NUMERIC=C;LC_TIME=ru_RU.UTF-8;LC_COLLATE=ru_RU.UTF-8;LC_MONETARY=ru_RU.UTF-8;LC_MESSAGES=ru_RU.UTF-8;LC_PAPER=ru_RU.UTF-8;LC_NAME=ru_RU.UTF-8;LC_ADDRESS=ru_RU.UTF-8;LC_TELEPHONE=ru_RU.UTF-8;LC_MEASUREMENT=ru_RU.UTF-8;LC_IDENTIFICATION=ru_RU.UTF-8
 */
static const gchar *
xkl_parse_LC_ALL_to_LC_MESSAGES(const gchar * lc_all)
{
	const gchar *lc_message_pos =
	    g_strstr_len(lc_all, -1, "LC_MESSAGES=");
	const gchar *lc_message_end;
	size_t len;
	static gchar buf[128];
	if (lc_message_pos == NULL)
		return lc_all;
	lc_message_pos += 12;
	lc_message_end = g_strstr_len(lc_message_pos, -1, ";");
	if (lc_message_end == NULL) {	/* LC_MESSAGES is the last piece of LC_ALL */
		return lc_message_pos;	/* safe to return! */
	}
	len = lc_message_end - lc_message_pos;
	if (len > sizeof(buf))
		len = sizeof(buf);
	g_strlcpy(buf, lc_message_pos, len);
	return buf;
}

gchar *
xkl_locale_from_utf8(XklConfigRegistry * config, const gchar * utf8string)
{
	const gchar *custom_charset =
	    xkl_config_registry_priv(config, custom_charset);
	return custom_charset ? g_convert(utf8string, -1, custom_charset,
					  "UTF-8", NULL, NULL, NULL)
	    : g_locale_from_utf8(utf8string, -1, NULL, NULL, NULL);
}

void
xkl_config_registry_set_custom_charset(XklConfigRegistry * config,
				       const gchar * charset)
{
	xkl_config_registry_priv(config, custom_charset) = charset;
}

/*
 * country[_LANG[.ENCODING]] - any other ideas?
 */
void
xkl_i18n_init(void)
{
	gchar *dot_pos;
	gchar *underscore_pos;
	const gchar *locale = NULL;
	gchar *cur_substring;

	locale_substrings[0][0] = locale_substrings[1][0] =
	    locale_substrings[2][0] = '\0';

#ifdef HAVE_SETLOCALE
	locale = setlocale(LC_MESSAGES, NULL);
#endif
	if (locale == NULL || locale[0] == '\0') {
		locale = getenv("LC_MESSAGES");
		if (locale == NULL || locale[0] == '\0') {
			locale = getenv("LC_ALL");
			if (locale == NULL || locale[0] == '\0')
				locale = getenv("LANG");
			else
				locale =
				    xkl_parse_LC_ALL_to_LC_MESSAGES
				    (locale);
		}
	}

	if (locale == NULL) {
		xkl_debug(0,
			  "Could not find locale - can be problems with i18n");
		return;
	}

	g_strlcpy(locale_substrings[0], locale, MAX_LOCALE_LEN);

	cur_substring = locale_substrings[1];

	dot_pos = g_strstr_len(locale, -1, ".");
	if (dot_pos != NULL) {
		gint idx = dot_pos - locale;
		if (idx >= MAX_LOCALE_LEN)
			idx = MAX_LOCALE_LEN - 1;
		g_strlcpy(cur_substring, locale, idx + 1);
		cur_substring += MAX_LOCALE_LEN;
	}

	underscore_pos = strchr(locale, '_');
	if (underscore_pos != NULL &&
	    (dot_pos == NULL || dot_pos > underscore_pos)) {
		gint idx = underscore_pos - locale;
		if (idx >= MAX_LOCALE_LEN)
			idx = MAX_LOCALE_LEN - 1;
		g_strlcpy(cur_substring, locale, idx + 1);
	}

	xkl_debug(150, "Locale search order:\n");
	/* full locale - highest priority */
	xkl_debug(150, " 0: %s\n", locale_substrings[0]);
	xkl_debug(150, " 1: %s\n", locale_substrings[1]);
	xkl_debug(150, " 2: %s\n", locale_substrings[2]);
}

gint
xkl_get_language_priority(const gchar * lang)
{
	gint i, priority = -1;

	for (i =
	     sizeof(locale_substrings) / sizeof(locale_substrings[0]);
	     --i >= 0;) {
		if (locale_substrings[0][0] == '\0')
			continue;

		if (!g_ascii_strcasecmp(lang, locale_substrings[i])) {
			priority = i;
			break;
		}
	}
	return priority;
}
