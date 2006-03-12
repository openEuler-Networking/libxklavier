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

static gchar locale_sub_strings[3][MAX_LOCALE_LEN];

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

/* Taken from gnome-vfs */
static gboolean
xkl_get_charset(const gchar ** a)
{
	static const gchar *charset = NULL;

	if (charset == NULL) {
		charset = g_getenv("CHARSET");

		if (charset == NULL || charset[0] == '\0') {
#ifdef HAVE_LANGINFO_CODESET
			charset = nl_langinfo(CODESET);
			if (charset == NULL || charset[0] == '\0') {
#endif
#ifdef HAVE_SETLOCALE
				charset = setlocale(LC_CTYPE, NULL);
				if (charset == NULL || charset[0] == '\0') {
#endif
					charset = getenv("LC_ALL");
					if (charset == NULL
					    || charset[0] == '\0') {
						charset =
						    getenv("LC_CTYPE");
						if (charset == NULL
						    || charset[0] == '\0')
							charset =
							    getenv("LANG");
					}
#ifdef HAVE_SETLOCALE
				} else {
					xkl_debug(150,
						  "Using charset from setlocale: [%s]\n",
						  charset);
				}
#endif
#ifdef HAVE_LANGINFO_CODESET
			} else {
				xkl_debug(150,
					  "Using charset from nl_langinfo: [%s]\n",
					  charset);
			}
#endif
		}
	}

	if (charset != NULL && *charset != '\0') {
		*a = charset;
		return (charset != NULL
			&& g_strstr_len(charset, -1, "UTF-8") != NULL);
	}
	/* Assume this for compatibility at present.  */
	*a = "US-ASCII";
	xkl_debug(150, "Using charset fallback: [%s]\n", *a);

	return FALSE;
}

gchar *
xkl_locale_from_utf8(const gchar * utf8string)
{
	size_t len;

	iconv_t converter;
	gchar converted[XKL_MAX_CI_DESC_LENGTH];
	gchar *converted_start = converted;
	gchar *utf_start = (char *) utf8string;
	size_t clen = XKL_MAX_CI_DESC_LENGTH - 1;
	const gchar *charset;

	static gboolean already_warned = FALSE;

	if (utf8string == NULL)
		return NULL;

	len = strlen(utf8string);

	if (xkl_get_charset(&charset))
		return g_strdup(utf8string);

	converter = iconv_open(charset, "UTF-8");
	if (converter == (iconv_t) - 1) {
		if (!already_warned) {
			already_warned = TRUE;
			xkl_debug(0,
				  "Unable to convert MIME info from UTF-8 "
				  "to the current locale %s. "
				  "MIME info will probably display wrong.",
				  charset);
		}
		return g_strdup(utf8string);
	}

	if (iconv(converter, &utf_start, &len, &converted_start, &clen) ==
	    -1) {
		xkl_debug(0,
			  "Unable to convert %s from UTF-8 to %s, "
			  "this string will probably display wrong.",
			  utf8string, charset);
		return g_strdup(utf8string);
	}
	*converted_start = '\0';

	iconv_close(converter);

	return g_strdup(converted);
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

	locale_sub_strings[0][0] = locale_sub_strings[1][0] =
	    locale_sub_strings[2][0] = '\0';

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

	g_strlcpy(locale_sub_strings[0], locale, MAX_LOCALE_LEN);

	cur_substring = locale_sub_strings[1];

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
	xkl_debug(150, " 0: %s\n", locale_sub_strings[0]);
	xkl_debug(150, " 1: %s\n", locale_sub_strings[1]);
	xkl_debug(150, " 2: %s\n", locale_sub_strings[2]);
}

gint
xkl_get_language_priority(const gchar * lang)
{
	gint i, priority = -1;

	for (i =
	     sizeof(locale_sub_strings) / sizeof(locale_sub_strings[0]);
	     --i >= 0;) {
		if (locale_sub_strings[0][0] == '\0')
			continue;

		if (!g_ascii_strcasecmp(lang, locale_sub_strings[i])) {
			priority = i;
			break;
		}
	}
	return priority;
}
