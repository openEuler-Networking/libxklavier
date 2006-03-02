#include <errno.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <fcntl.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

#ifdef XKB_HEADERS_PRESENT
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>
#endif

/* For "bad" X servers we hold our own copy */
#define XML_CFG_FALLBACK_PATH ( DATA_DIR "/xfree86.xml" )

#define XKBCOMP ( XKB_BIN_BASE "/xkbcomp" )

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#ifdef XKB_HEADERS_PRESENT
static XkbRF_RulesPtr xkl_rules;

static XkbRF_RulesPtr
xkl_rules_set_load(XklEngine * engine)
{
	XkbRF_RulesPtr rules_set = NULL;
	char file_name[MAXPATHLEN] = "";
	char *rf =
	    xkl_engine_get_ruleset_name(engine, XKB_DEFAULT_RULESET);
	char *locale = NULL;

	if (rf == NULL) {
		xkl_last_error_message =
		    "Could not find the XKB rules set";
		return NULL;
	}

	locale = setlocale(LC_ALL, NULL);

	snprintf(file_name, sizeof file_name, XKB_BASE "/rules/%s", rf);
	xkl_debug(160, "Loading rules from [%s]\n", file_name);

	rules_set = XkbRF_Load(file_name, locale, True, True);

	if (rules_set == NULL) {
		xkl_last_error_message = "Could not load rules";
		return NULL;
	}
	return rules_set;
}

static void
xkl_rules_set_free(void)
{
	if (xkl_rules)
		XkbRF_Free(xkl_rules, True);
	xkl_rules = NULL;
}
#endif

void
xkl_xkb_init_config_registry(XklConfigRegistry * config)
{
#ifdef XKB_HEADERS_PRESENT
	XkbInitAtoms(NULL);
#endif
}

gboolean
xkl_xkb_load_config_registry(XklConfigRegistry * config)
{
	struct stat stat_buf;
	char file_name[MAXPATHLEN] = "";
	char *rf =
	    xkl_engine_get_ruleset_name(xkl_config_registry_get_engine
					(config),
					XKB_DEFAULT_RULESET);

	if (rf == NULL)
		return FALSE;

	snprintf(file_name, sizeof file_name, XKB_BASE "/rules/%s.xml",
		 rf);

	if (stat(file_name, &stat_buf) != 0) {
		g_strlcpy(file_name, XML_CFG_FALLBACK_PATH,
			  sizeof file_name);
	}

	return xkl_config_registry_load_from_file(config, file_name);
}

#ifdef XKB_HEADERS_PRESENT
gboolean
xkl_xkb_config_native_prepare(XklEngine * engine,
			      const XklConfigRec * data,
			      XkbComponentNamesPtr component_names_ptr)
{
	XkbRF_VarDefsRec xkl_var_defs;
	gboolean got_components;

	memset(&xkl_var_defs, 0, sizeof(xkl_var_defs));

	xkl_rules = xkl_rules_set_load(engine);
	if (!xkl_rules) {
		return FALSE;
	}

	xkl_var_defs.model = (char *) data->model;

	if (data->layouts != NULL)
		xkl_var_defs.layout = xkl_config_rec_merge_layouts(data);

	if (data->variants != NULL)
		xkl_var_defs.variant = xkl_config_rec_merge_variants(data);

	if (data->options != NULL)
		xkl_var_defs.options = xkl_config_rec_merge_options(data);

	got_components =
	    XkbRF_GetComponents(xkl_rules, &xkl_var_defs,
				component_names_ptr);

	g_free(xkl_var_defs.layout);
	g_free(xkl_var_defs.variant);
	g_free(xkl_var_defs.options);

	if (!got_components) {
		xkl_last_error_message =
		    "Could not translate rules into components";
		/* Just cleanup the stuff in case of failure */
		xkl_xkb_config_native_cleanup(engine, component_names_ptr);

		return FALSE;
	}

	if (xkl_debug_level >= 200) {
		xkl_debug(200, "keymap: %s\n",
			  component_names_ptr->keymap);
		xkl_debug(200, "keycodes: %s\n",
			  component_names_ptr->keycodes);
		xkl_debug(200, "compat: %s\n",
			  component_names_ptr->compat);
		xkl_debug(200, "types: %s\n", component_names_ptr->types);
		xkl_debug(200, "symbols: %s\n",
			  component_names_ptr->symbols);
		xkl_debug(200, "geometry: %s\n",
			  component_names_ptr->geometry);
	}
	return TRUE;
}

void
xkl_xkb_config_native_cleanup(XklEngine * engine,
			      XkbComponentNamesPtr component_names_ptr)
{
	xkl_rules_set_free();

	g_free(component_names_ptr->keymap);
	g_free(component_names_ptr->keycodes);
	g_free(component_names_ptr->compat);
	g_free(component_names_ptr->types);
	g_free(component_names_ptr->symbols);
	g_free(component_names_ptr->geometry);
}

static XkbDescPtr
xkl_config_get_keyboard(XklEngine * engine,
			XkbComponentNamesPtr component_names_ptr,
			gboolean activate)
{
	XkbDescPtr xkb = NULL;
#if 0
	xkb = XkbGetKeyboardByName(_xklDpy,
				   XkbUseCoreKbd,
				   &componentNames,
				   XkbGBN_AllComponentsMask &
				   (~XkbGBN_GeometryMask),
				   XkbGBN_AllComponentsMask &
				   (~XkbGBN_GeometryMask), activate);
#else
	char xkm_fn[L_tmpnam];
	char xkb_fn[L_tmpnam];
	FILE *tmpxkm;
	XkbFileInfo result;
	int xkmloadres;

	Display *display = xkl_engine_get_display(engine);

	if (tmpnam(xkm_fn) != NULL && tmpnam(xkb_fn) != NULL) {
		pid_t cpid, pid;
		int status = 0;
		FILE *tmpxkb;

		xkl_debug(150, "tmp XKB/XKM file names: [%s]/[%s]\n",
			  xkb_fn, xkm_fn);
		if ((tmpxkb = fopen(xkb_fn, "w")) != NULL) {
			fprintf(tmpxkb, "xkb_keymap {\n");
			fprintf(tmpxkb,
				"        xkb_keycodes  { include \"%s\" };\n",
				component_names_ptr->keycodes);
			fprintf(tmpxkb,
				"        xkb_types     { include \"%s\" };\n",
				component_names_ptr->types);
			fprintf(tmpxkb,
				"        xkb_compat    { include \"%s\" };\n",
				component_names_ptr->compat);
			fprintf(tmpxkb,
				"        xkb_symbols   { include \"%s\" };\n",
				component_names_ptr->symbols);
			fprintf(tmpxkb,
				"        xkb_geometry  { include \"%s\" };\n",
				component_names_ptr->geometry);
			fprintf(tmpxkb, "};\n");
			fclose(tmpxkb);

			xkl_debug(150, "xkb_keymap {\n"
				  "        xkb_keycodes  { include \"%s\" };\n"
				  "        xkb_types     { include \"%s\" };\n"
				  "        xkb_compat    { include \"%s\" };\n"
				  "        xkb_symbols   { include \"%s\" };\n"
				  "        xkb_geometry  { include \"%s\" };\n};\n",
				  component_names_ptr->keycodes,
				  component_names_ptr->types,
				  component_names_ptr->compat,
				  component_names_ptr->symbols,
				  component_names_ptr->geometry);

			cpid = fork();
			switch (cpid) {
			case -1:
				xkl_debug(0, "Could not fork: %d\n",
					  errno);
				break;
			case 0:
				/* child */
				xkl_debug(160, "Executing %s\n", XKBCOMP);
				xkl_debug(160, "%s %s %s %s %s %s %s\n",
					  XKBCOMP, XKBCOMP, "-I",
					  "-I" XKB_BASE, "-xkm", xkb_fn,
					  xkm_fn);
				execl(XKBCOMP, XKBCOMP, "-I",
				      "-I" XKB_BASE, "-xkm", xkb_fn,
				      xkm_fn, NULL);
				xkl_debug(0, "Could not exec %s: %d\n",
					  XKBCOMP, errno);
				exit(1);
			default:
				/* parent */
				pid = waitpid(cpid, &status, 0);
				xkl_debug(150,
					  "Return status of %d (well, started %d): %d\n",
					  pid, cpid, status);
				memset((char *) &result, 0,
				       sizeof(result));
				result.xkb = XkbAllocKeyboard();

				if (Success ==
				    XkbChangeKbdDisplay(display,
							&result)) {
					xkl_debug(150,
						  "Hacked the kbddesc - set the display...\n");
					if ((tmpxkm =
					     fopen(xkm_fn, "r")) != NULL) {
						xkmloadres =
						    XkmReadFile(tmpxkm,
								XkmKeymapLegal,
								XkmKeymapLegal,
								&result);
						xkl_debug(150,
							  "Loaded %s output as XKM file, got %d (comparing to %d)\n",
							  XKBCOMP,
							  (int) xkmloadres,
							  (int)
							  XkmKeymapLegal);
						if ((int) xkmloadres !=
						    (int) XkmKeymapLegal) {
							xkl_debug(150,
								  "Loaded legal keymap\n");
							if (activate) {
								xkl_debug
								    (150,
								     "Activating it...\n");
								if (XkbWriteToServer(&result)) {
									xkl_debug
									    (150,
									     "Updating the keyboard...\n");
									xkb = result.xkb;
								} else {
									xkl_debug
									    (0,
									     "Could not write keyboard description to the server\n");
								}
							} else	/* no activate, just load */
								xkb =
								    result.
								    xkb;
						} else {	/* could not load properly */

							xkl_debug(0,
								  "Could not load %s output as XKM file, got %d (asked %d)\n",
								  XKBCOMP,
								  (int)
								  xkmloadres,
								  (int)
								  XkmKeymapLegal);
						}
						fclose(tmpxkm);
						xkl_debug(160,
							  "Unlinking the temporary xkm file %s\n",
							  xkm_fn);
						if (xkl_debug_level < 500) {	/* don't remove on high debug levels! */
							if (remove(xkm_fn)
							    == -1)
								xkl_debug
								    (0,
								     "Could not unlink the temporary xkm file %s: %d\n",
								     xkm_fn,
								     errno);
						} else
							xkl_debug(500,
								  "Well, not really - the debug level is too high: %d\n",
								  xkl_debug_level);
					} else {	/* could not open the file */

						xkl_debug(0,
							  "Could not open the temporary xkm file %s\n",
							  xkm_fn);
					}
				} else {	/* could not assign to display */

					xkl_debug(0,
						  "Could not change the keyboard description to display\n");
				}
				if (xkb == NULL)
					XkbFreeKeyboard(result.xkb,
							XkbAllComponentsMask,
							True);
				break;
			}
			xkl_debug(160,
				  "Unlinking the temporary xkb file %s\n",
				  xkb_fn);
			if (xkl_debug_level < 500) {	/* don't remove on high debug levels! */
				if (remove(xkb_fn) == -1)
					xkl_debug(0,
						  "Could not unlink the temporary xkb file %s: %d\n",
						  xkb_fn, errno);
			} else
				xkl_debug(500,
					  "Well, not really - the debug level is too high: %d\n",
					  xkl_debug_level);
		} else {	/* could not open input tmp file */

			xkl_debug(0,
				  "Could not open tmp XKB file [%s]: %d\n",
				  xkb_fn, errno);
		}
	} else {
		xkl_debug(0, "Could not get tmp names\n");
	}

#endif
	return xkb;
}
#else				/* no XKB headers */
gboolean
xkl_xkb_config_native_prepare(const XklConfigRec * data,
			      gpointer componentNamesPtr)
{
	return FALSE;
}

void
_XklXkbConfigCleanupNative(gpointer componentNamesPtr)
{
}
#endif

/* check only client side support */
gboolean
xkl_xkb_multiple_layouts_supported(XklEngine * engine)
{
	enum { NON_SUPPORTED, SUPPORTED, UNCHECKED };

	static int support_state = UNCHECKED;

	if (support_state == UNCHECKED) {
		XklConfigRec data;
		char *layouts[] = { "us", "de", NULL };
		char *variants[] = { NULL, NULL, NULL };
#ifdef XKB_HEADERS_PRESENT
		XkbComponentNamesRec component_names;
		memset(&component_names, 0, sizeof(component_names));
#endif

		data.model = "pc105";
		data.layouts = layouts;
		data.variants = variants;
		data.options = NULL;

		xkl_debug(100, "!!! Checking multiple layouts support\n");
		support_state = NON_SUPPORTED;
#ifdef XKB_HEADERS_PRESENT
		if (xkl_xkb_config_native_prepare
		    (engine, &data, &component_names)) {
			xkl_debug(100,
				  "!!! Multiple layouts ARE supported\n");
			support_state = SUPPORTED;
			xkl_xkb_config_native_cleanup(engine,
						      &component_names);
		} else {
			xkl_debug(100,
				  "!!! Multiple layouts ARE NOT supported\n");
		}
#endif
	}
	return support_state == SUPPORTED;
}

gboolean
xkl_xkb_activate_config_rec(XklEngine * engine, const XklConfigRec * data)
{
	gboolean rv = FALSE;
#if 0
	{
		int i;
		XklDebug(150, "New model: [%s]\n", data->model);
		XklDebug(150, "New layouts: %p\n", data->layouts);
		for (i = data->numLayouts; --i >= 0;)
			XklDebug(150, "New layout[%d]: [%s]\n", i,
				 data->layouts[i]);
		XklDebug(150, "New variants: %p\n", data->variants);
		for (i = data->numVariants; --i >= 0;)
			XklDebug(150, "New variant[%d]: [%s]\n", i,
				 data->variants[i]);
		XklDebug(150, "New options: %p\n", data->options);
		for (i = data->numOptions; --i >= 0;)
			XklDebug(150, "New option[%d]: [%s]\n", i,
				 data->options[i]);
	}
#endif

#ifdef XKB_HEADERS_PRESENT
	XkbComponentNamesRec component_names;
	memset(&component_names, 0, sizeof(component_names));

	if (xkl_xkb_config_native_prepare(engine, data, &component_names)) {
		XkbDescPtr xkb;
		xkb =
		    xkl_config_get_keyboard(engine, &component_names,
					    TRUE);
		if (xkb != NULL) {
			if (xkl_config_rec_set_to_root_window_property
			    (data,
			     xkl_engine_priv(engine, base_config_atom),
			     xkl_engine_get_ruleset_name(engine,
							 XKB_DEFAULT_RULESET),
			     engine))
				/* We do not need to check the result of _XklGetRulesSetName - 
				   because PrepareBeforeKbd did it for us */
				rv = TRUE;
			else
				xkl_last_error_message =
				    "Could not set names property";
			XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);
		} else {
			xkl_last_error_message =
			    "Could not load keyboard description";
		}
		xkl_xkb_config_native_cleanup(engine, &component_names);
	}
#endif
	return rv;
}

gboolean
xkl_xkb_write_config_rec_to_file(XklEngine * engine, const char *file_name,
				 const XklConfigRec * data,
				 const gboolean binary)
{
	gboolean rv = FALSE;

#ifdef XKB_HEADERS_PRESENT
	XkbComponentNamesRec component_names;
	FILE *output = fopen(file_name, "w");
	XkbFileInfo dump_info;

	if (output == NULL) {
		xkl_last_error_message = "Could not open the XKB file";
		return FALSE;
	}

	memset(&component_names, 0, sizeof(component_names));

	if (xkl_xkb_config_native_prepare(engine, data, &component_names)) {
		XkbDescPtr xkb;
		xkb =
		    xkl_config_get_keyboard(engine, &component_names,
					    FALSE);
		if (xkb != NULL) {
			dump_info.defined = 0;
			dump_info.xkb = xkb;
			dump_info.type = XkmKeymapFile;
			if (binary)
				rv = XkbWriteXKMFile(output, &dump_info);
			else
				rv = XkbWriteXKBFile(output, &dump_info,
						     True, NULL, NULL);

			XkbFreeKeyboard(xkb, XkbGBN_AllComponentsMask,
					True);
		} else
			xkl_last_error_message =
			    "Could not load keyboard description";
		xkl_xkb_config_native_cleanup(engine, &component_names);
	}
	fclose(output);
#endif
	return rv;
}
