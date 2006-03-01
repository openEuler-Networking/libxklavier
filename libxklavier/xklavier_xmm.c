#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

#define SHORTCUT_OPTION_PREFIX "grp:"

gchar *current_xmm_rules = NULL;

XklConfigRec current_xmm_config;

Atom xmm_state_atom;

const gchar **
xkl_xmm_get_groups_names(XklEngine * engine)
{
	return (const gchar **) current_xmm_config.layouts;
}

void
xkl_xmm_shortcuts_grab(XklEngine * engine)
{
	const XmmShortcut *shortcut;
	const XmmSwitchOption *option = xkl_xmm_shortcut_get_current();

	xkl_debug(150, "Found shortcut option: %p\n", option);
	if (option == NULL)
		return;

	shortcut = option->shortcuts;
	while (shortcut->keysym != XK_VoidSymbol) {
		int keycode =
		    XKeysymToKeycode(xkl_engine_get_display(engine),
				     shortcut->keysym);
		xkl_xmm_grab_ignoring_indicators(engine, keycode,
						 shortcut->modifiers);
		shortcut++;
	}
}

void
xkl_xmm_shortcuts_ungrab(XklEngine * engine)
{
	const XmmShortcut *shortcut;
	const XmmSwitchOption *option = xkl_xmm_shortcut_get_current();

	if (option == NULL)
		return;

	shortcut = option->shortcuts;
	while (shortcut->keysym != XK_VoidSymbol) {
		int keycode =
		    XKeysymToKeycode(xkl_engine_get_display(engine),
				     shortcut->keysym);
		xkl_xmm_ungrab_ignoring_indicators(engine, keycode,
						   shortcut->modifiers);
		shortcut++;
	}
}

XmmSwitchOption *
xkl_xmm_shortcut_get_current(void)
{
	const gchar *option_name =
	    xkl_xmm_shortcut_get_current_option_name();
	XmmSwitchOption *switch_option = all_switch_options;
	xkl_debug(150, "Configured switch option: [%s]\n", option_name);
	if (option_name == NULL)
		return NULL;
	while (switch_option->option_name != NULL) {
		if (!g_ascii_strcasecmp
		    (switch_option->option_name, option_name))
			return switch_option;
		switch_option++;
	}
	return NULL;
}

const gchar *
xkl_xmm_shortcut_get_current_option_name(void)
{
	gchar **option = current_xmm_config.options;
	if (option == NULL)
		return NULL;

	while (*option != NULL) {
		/* starts with "grp:" */
		if (strstr(*option, SHORTCUT_OPTION_PREFIX) != NULL) {
			return *option + sizeof SHORTCUT_OPTION_PREFIX - 1;
		}
		option++;
	}
	return NULL;
}

const XmmSwitchOption *
xkl_xmm_find_switch_option(XklEngine * engine, gint keycode,
			   guint state, gint * current_shortcut_rv)
{
	const XmmSwitchOption *rv = xkl_xmm_shortcut_get_current();

	if (rv != NULL) {
		XmmShortcut *sc = rv->shortcuts;
		while (sc->keysym != XK_VoidSymbol) {
			if ((XKeysymToKeycode
			     (xkl_engine_get_display(engine),
			      sc->keysym) == keycode)
			    && ((state & sc->modifiers) == sc->modifiers)) {
				return rv;
			}
			sc++;
		}
	}
	return NULL;
}

gint
xkl_xmm_resume_listen(XklEngine * engine)
{
	if (engine->priv->listener_type & XKLL_MANAGE_LAYOUTS)
		xkl_xmm_shortcuts_grab(engine);
	return 0;
}

gint
xkl_xmm_pause_listen(XklEngine * engine)
{
	if (engine->priv->listener_type & XKLL_MANAGE_LAYOUTS)
		xkl_xmm_shortcuts_ungrab(engine);
	return 0;
}

guint
xkl_xmm_get_max_num_groups(XklEngine * engine)
{
	return 0;
}

guint
xkl_xmm_get_num_groups(XklEngine * engine)
{
	gint rv = 0;
	gchar **p = current_xmm_config.layouts;
	if (p != NULL)
		while (*p++ != NULL)
			rv++;
	return rv;
}

void
xkl_xmm_free_all_info(XklEngine * engine)
{
	if (current_xmm_rules != NULL) {
		g_free(current_xmm_rules);
		current_xmm_rules = NULL;
	}
	xkl_config_rec_reset(&current_xmm_config);
}

gboolean
xkl_xmm_if_cached_info_equals_actual(XklEngine * engine)
{
	return FALSE;
}

gboolean
xkl_xmm_load_all_info(XklEngine * engine)
{
	return xkl_config_rec_get_full_from_server(&current_xmm_rules,
						   &current_xmm_config,
						   engine);
}

void
xkl_xmm_get_server_state(XklEngine * engine, XklState * state)
{
	unsigned char *propval = NULL;
	Atom actual_type;
	int actual_format;
	unsigned long bytes_remaining;
	unsigned long actual_items;
	int result;

	memset(state, 0, sizeof(*state));

	result =
	    XGetWindowProperty(xkl_engine_get_display(engine),
			       engine->priv->root_window, xmm_state_atom,
			       0L, 1L, False, XA_INTEGER, &actual_type,
			       &actual_format, &actual_items,
			       &bytes_remaining, &propval);

	if (Success == result) {
		if (actual_format == 32 || actual_items == 1) {
			state->group = *(CARD32 *) propval;
		} else {
			xkl_debug(160,
				  "Could not get the xmodmap current group\n");
		}
		XFree(propval);
	} else {
		xkl_debug(160,
			  "Could not get the xmodmap current group: %d\n",
			  result);
	}
}

void
xkl_xmm_group_actualize(XklEngine * engine, gint group)
{
	char cmd[1024];
	int res;
	const gchar *layout_name = NULL;

	if (xkl_xmm_get_num_groups(engine) < group)
		return;

	layout_name = current_xmm_config.layouts[group];

	snprintf(cmd, sizeof cmd,
		 "xmodmap %s/xmodmap.%s", XMODMAP_BASE, layout_name);

	res = system(cmd);
	if (res > 0) {
		xkl_debug(0, "xmodmap error %d\n", res);
	} else if (res < 0) {
		xkl_debug(0, "Could not execute xmodmap: %d\n", res);
	}
	XSync(xkl_engine_get_display(engine), False);
}

void
xkl_xmm_lock_group(XklEngine * engine, gint group)
{
	CARD32 propval;

	if (xkl_xmm_get_num_groups(engine) < group)
		return;

	/* updating the status property */
	propval = group;
	Display *display = xkl_engine_get_display(engine);
	XChangeProperty(display, engine->priv->root_window, xmm_state_atom,
			XA_INTEGER, 32, PropModeReplace,
			(unsigned char *) &propval, 1);
	XSync(display, False);
}

gint
xkl_xmm_init(XklEngine * engine)
{
	engine->priv->backend_id = "xmodmap";
	engine->priv->features = XKLF_MULTIPLE_LAYOUTS_SUPPORTED |
	    XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT;
	engine->priv->activate_config = xkl_xmm_activate_config;
	engine->priv->init_config = xkl_xmm_init_config;
	engine->priv->load_config_registry = xkl_xmm_load_config_registry;
	engine->priv->write_config_to_file = NULL;

	engine->priv->get_groups_names = xkl_xmm_get_groups_names;
	engine->priv->get_max_num_groups = xkl_xmm_get_max_num_groups;
	engine->priv->get_num_groups = xkl_xmm_get_num_groups;
	engine->priv->lock_group = xkl_xmm_lock_group;

	engine->priv->process_x_event = xkl_xmm_process_x_event;
	engine->priv->free_all_info = xkl_xmm_free_all_info;
	engine->priv->if_cached_info_equals_actual =
	    xkl_xmm_if_cached_info_equals_actual;
	engine->priv->load_all_info = xkl_xmm_load_all_info;
	engine->priv->get_server_state = xkl_xmm_get_server_state;
	engine->priv->pause_listen = xkl_xmm_pause_listen;
	engine->priv->resume_listen = xkl_xmm_resume_listen;
	engine->priv->set_indicators = NULL;

	if (getenv("XKL_XMODMAP_DISABLE") != NULL)
		return -1;

	Display *display = xkl_engine_get_display(engine);
	engine->priv->base_config_atom =
	    XInternAtom(display, "_XMM_NAMES", False);
	engine->priv->backup_config_atom =
	    XInternAtom(display, "_XMM_NAMES_BACKUP", False);

	xmm_state_atom = XInternAtom(display, "_XMM_STATE", False);

	engine->priv->default_model = "generic";
	engine->priv->default_layout = "us";

	return 0;
}
