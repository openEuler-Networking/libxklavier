#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

static XklEngine *the_engine = NULL;

gint xkl_debug_level = 0;

static XklLogAppender log_appender = xkl_default_log_appender;


void
xkl_engine_set_indicators_handling(XklEngine * engine,
				   gboolean whether_handle)
{
	engine->priv->handle_indicators = whether_handle;
}

gboolean
xkl_engine_get_indicators_handling(XklEngine * engine)
{
	return engine->priv->handle_indicators;
}

void
xkl_set_debug_level(int level)
{
	xkl_debug_level = level;
}

void
xkl_engine_set_group_per_toplevel_window(XklEngine * engine,
					 gboolean is_set)
{
	engine->priv->group_per_toplevel_window = is_set;
}

gboolean
xkl_engine_is_group_per_toplevel_window(XklEngine * engine)
{
	return engine->priv->group_per_toplevel_window;
}

static void
xkl_engine_set_switch_to_secondary_group(XklEngine * engine, gboolean val)
{
	CARD32 propval = (CARD32) val;
	Display *dpy = xkl_engine_get_display(engine);
	XChangeProperty(dpy,
			engine->priv->root_window,
			engine->priv->atoms[XKLAVIER_ALLOW_SECONDARY],
			XA_INTEGER, 32, PropModeReplace,
			(unsigned char *) &propval, 1);
	XSync(dpy, False);
}

void
xkl_engine_allow_one_switch_to_secondary_group(XklEngine * engine)
{
	xkl_debug(150,
		  "Setting allow_one_switch_to_secondary_group flag\n");
	xkl_engine_set_switch_to_secondary_group(engine, TRUE);
}

gboolean
xkl_engine_is_one_switch_to_secondary_group_allowed(XklEngine * engine)
{
	gboolean rv = FALSE;
	unsigned char *propval = NULL;
	Atom actual_type;
	int actual_format;
	unsigned long bytes_remaining;
	unsigned long actual_items;
	int result;

	result =
	    XGetWindowProperty(xkl_engine_get_display(engine),
			       engine->priv->root_window,
			       engine->priv->
			       atoms[XKLAVIER_ALLOW_SECONDARY], 0L, 1L,
			       False, XA_INTEGER, &actual_type,
			       &actual_format, &actual_items,
			       &bytes_remaining, &propval);

	if (Success == result) {
		if (actual_format == 32 && actual_items == 1) {
			rv = (gboolean) * (Bool *) propval;
		}
		XFree(propval);
	}

	return rv;
}

void
xkl_engine_one_switch_to_secondary_group_performed(XklEngine * engine)
{
	xkl_debug(150,
		  "Resetting allow_one_switch_to_secondary_group flag\n");
	xkl_engine_set_switch_to_secondary_group(engine, FALSE);
}

void
xkl_engine_set_default_group(XklEngine * engine, gint group)
{
	engine->priv->default_group = group;
}

gint
xkl_engine_get_default_group(XklEngine * engine)
{
	return engine->priv->default_group;
}

void
xkl_engine_set_secondary_groups_mask(XklEngine * engine, guint mask)
{
	engine->priv->secondary_groups_mask = mask;
}

guint
xkl_engine_get_secondary_groups_mask(XklEngine * engine)
{
	return engine->priv->secondary_groups_mask;
}

void
xkl_set_log_appender(XklLogAppender func)
{
	log_appender = func;
}

gint
xkl_engine_start_listen(XklEngine * engine, guint what)
{
	engine->priv->listener_type = what;

	if (!
	    (engine->priv->
	     features & XKLF_REQUIRES_MANUAL_LAYOUT_MANAGEMENT)
&& (what & XKLL_MANAGE_LAYOUTS))
		xkl_debug(0,
			  "The backend does not require manual layout management - "
			  "but it is provided by the application");

	xkl_engine_resume_listen(engine);
	xkl_engine_load_window_tree(engine);
	XFlush(xkl_engine_get_display(engine));
	return 0;
}

gint
xkl_engine_stop_listen(XklEngine * engine)
{
	xkl_engine_pause_listen(engine);
	return 0;
}

XklEngine *
xkl_engine_get_instance(Display * display)
{
	if (the_engine != NULL) {
		g_object_ref(G_OBJECT(the_engine));
		return the_engine;
	}

	if (!display) {
		xkl_debug(10, "xkl_init : display is NULL ?\n");
		return NULL;
	}

	the_engine = XKL_ENGINE(g_object_new(xkl_engine_get_type(), NULL));


	the_engine->priv->display = display;

	const gchar *sdl = g_getenv("XKL_DEBUG");

	if (sdl != NULL) {
		xkl_set_debug_level(atoi(sdl));
	}

	gint rv = -1;
	xkl_debug(150, "Trying all backends:\n");
#ifdef ENABLE_XKB_SUPPORT
	xkl_debug(150, "Trying XKB backend\n");
	rv = xkl_xkb_init(the_engine);
#endif
#ifdef ENABLE_XMM_SUPPORT
	if (rv != 0) {
		xkl_debug(150, "Trying XMM backend\n");
		rv = xkl_xmm_init(the_engine);
	}
#endif
	if (rv == 0) {
		xkl_debug(150, "Actual backend: %s\n",
			  xkl_engine_get_backend_name(the_engine));
	} else {
		xkl_debug(0, "All backends failed, last result: %d\n", rv);
		the_engine->priv->display = NULL;
		g_object_unref(G_OBJECT(the_engine));
		the_engine = NULL;
		return NULL;
	}

	int scr;

	the_engine->priv->default_error_handler =
	    XSetErrorHandler((XErrorHandler) xkl_process_error);

	Display *dpy = xkl_engine_get_display(the_engine);
	scr = DefaultScreen(dpy);
	the_engine->priv->root_window = RootWindow(dpy, scr);

	the_engine->priv->skip_one_restore = FALSE;
	the_engine->priv->default_group = -1;
	the_engine->priv->secondary_groups_mask = 0L;
	the_engine->priv->prev_toplvl_win = 0;

	the_engine->priv->atoms[WM_NAME] =
	    XInternAtom(dpy, "WM_NAME", False);
	the_engine->priv->atoms[WM_STATE] =
	    XInternAtom(dpy, "WM_STATE", False);
	the_engine->priv->atoms[XKLAVIER_STATE] =
	    XInternAtom(dpy, "XKLAVIER_STATE", False);
	the_engine->priv->atoms[XKLAVIER_TRANSPARENT] =
	    XInternAtom(dpy, "XKLAVIER_TRANSPARENT", False);
	the_engine->priv->atoms[XKLAVIER_ALLOW_SECONDARY] =
	    XInternAtom(dpy, "XKLAVIER_ALLOW_SECONDARY", False);

	xkl_engine_one_switch_to_secondary_group_performed(the_engine);

	if (!xkl_engine_load_all_info(the_engine)) {
		g_object_unref(G_OBJECT(the_engine));
		the_engine = NULL;
	}

	return the_engine;
}

gint
xkl_term(XklEngine * engine)
{
	XSetErrorHandler((XErrorHandler) engine->priv->
			 default_error_handler);

	xkl_engine_free_all_info(engine);

	return 0;
}

gboolean
xkl_engine_grab_key(XklEngine * engine, gint keycode, guint modifiers)
{
	gboolean ret_code;
	gchar *keyname;
	Display *dpy = xkl_engine_get_display(engine);

	if (xkl_debug_level >= 100) {
		keyname =
		    XKeysymToString(XKeycodeToKeysym(dpy, keycode, 0));
		xkl_debug(100, "Listen to the key %d/(%s)/%d\n", keycode,
			  keyname, modifiers);
	}

	if (0 == keycode)
		return FALSE;

	engine->priv->last_error_code = Success;

	ret_code =
	    XGrabKey(dpy, keycode, modifiers, engine->priv->root_window,
		     TRUE, GrabModeAsync, GrabModeAsync);
	XSync(dpy, False);

	xkl_debug(100, "XGrabKey recode %d/error %d\n",
		  ret_code, engine->priv->last_error_code);

	ret_code = (engine->priv->last_error_code == Success);

	if (!ret_code)
		engine->priv->last_error_message =
		    "Could not grab the key";

	return ret_code;
}

gboolean
xkl_engine_ungrab_key(XklEngine * engine, gint keycode, guint modifiers)
{
	if (0 == keycode)
		return FALSE;

	return Success == XUngrabKey(xkl_engine_get_display(engine),
				     keycode, 0,
				     engine->priv->root_window);
}

gint
xkl_engine_get_next_group(XklEngine * engine)
{
	gint n = xkl_engine_get_num_groups(engine);
	return (engine->priv->curr_state.group + 1) % n;
}

gint
xkl_engine_get_prev_group(XklEngine * engine)
{
	gint n = xkl_engine_get_num_groups(engine);
	return (engine->priv->curr_state.group + n - 1) % n;
}

gint
xkl_engine_get_current_window_group(XklEngine * engine)
{
	XklState state;
	if (engine->priv->curr_toplvl_win == (Window) NULL) {
		xkl_debug(150, "cannot restore without current client\n");
	} else
	    if (xkl_engine_get_toplevel_window_state
		(engine, engine->priv->curr_toplvl_win, &state)) {
		return state.group;
	} else
		xkl_debug(150,
			  "Unbelievable: current client " WINID_FORMAT
			  ", '%s' has no group\n",
			  engine->priv->curr_toplvl_win,
			  xkl_get_debug_window_title(engine, engine->priv->
						     curr_toplvl_win));
	return 0;
}

void
xkl_engine_set_transparent(XklEngine * engine, Window win,
			   gboolean transparent)
{
	Window toplevel_win;
	xkl_debug(150,
		  "setting transparent flag %d for " WINID_FORMAT "\n",
		  transparent, win);

	if (!xkl_engine_find_toplevel_window(engine, win, &toplevel_win)) {
		xkl_debug(150, "No toplevel window!\n");
		toplevel_win = win;
/*    return; */
	}

	xkl_engine_set_toplevel_window_transparent(engine, toplevel_win,
						   transparent);
}

gboolean
xkl_engine_is_window_transparent(XklEngine * engine, Window win)
{
	Window toplevel_win;

	if (!xkl_engine_find_toplevel_window(engine, win, &toplevel_win))
		return FALSE;
	return xkl_engine_is_toplevel_window_transparent(engine,
							 toplevel_win);
}

/**
 * Loads the tree recursively.
 */
gboolean
xkl_engine_load_window_tree(XklEngine * engine)
{
	Window focused;
	int revert;
	gboolean retval = TRUE, have_toplevel_win;

	if (engine->priv->listener_type & XKLL_MANAGE_WINDOW_STATES)
		retval =
		    xkl_engine_load_subtree(engine,
					    engine->priv->root_window, 0,
					    &engine->priv->curr_state);

	XGetInputFocus(xkl_engine_get_display(engine), &focused, &revert);

	xkl_debug(160, "initially focused: " WINID_FORMAT ", '%s'\n",
		  focused, xkl_get_debug_window_title(engine, focused));

	have_toplevel_win =
	    xkl_engine_find_toplevel_window(engine, focused,
					    &engine->priv->
					    curr_toplvl_win);

	if (have_toplevel_win) {
		gboolean have_state =
		    xkl_engine_get_toplevel_window_state(engine,
							 engine->priv->
							 curr_toplvl_win,
							 &engine->priv->
							 curr_state);
		xkl_debug(160,
			  "initial toplevel: " WINID_FORMAT
			  ", '%s' %s state %d/%X\n",
			  engine->priv->curr_toplvl_win,
			  xkl_get_debug_window_title(engine, engine->priv->
						     curr_toplvl_win),
			  (have_state ? "with" : "without"),
			  (have_state ? engine->priv->curr_state.
			   group : -1),
			  (have_state ? engine->priv->curr_state.
			   indicators : -1));
	} else {
		xkl_debug(160,
			  "Could not find initial app. "
			  "Probably, focus belongs to some WM service window. "
			  "Will try to survive:)");
	}

	return retval;
}

void
_xkl_debug(const gchar file[], const gchar function[], gint level,
	   const gchar format[], ...)
{
	va_list lst;

	if (level > xkl_debug_level)
		return;

	va_start(lst, format);
	if (log_appender != NULL)
		(*log_appender) (file, function, level, format, lst);
	va_end(lst);
}

void
xkl_default_log_appender(const gchar file[], const gchar function[],
			 gint level, const gchar format[], va_list args)
{
	time_t now = time(NULL);
	fprintf(stdout, "[%08ld,%03d,%s:%s/] \t", now, level, file,
		function);
	vfprintf(stdout, format, args);
}

/**
 * Just selects some events from the window.
 */
void
xkl_engine_select_input(XklEngine * engine, Window win, gulong mask)
{
	if (engine->priv->root_window == win)
		xkl_debug(160,
			  "Someone is looking for %lx on root window ***\n",
			  mask);

	XSelectInput(xkl_engine_get_display(engine), win, mask);
}

void
xkl_engine_select_input_merging(XklEngine * engine, Window win,
				gulong mask)
{
	XWindowAttributes attrs;
	gulong oldmask = 0L, newmask;
	memset(&attrs, 0, sizeof(attrs));
	if (XGetWindowAttributes
	    (xkl_engine_get_display(engine), win, &attrs))
		oldmask = attrs.your_event_mask;

	newmask = oldmask | mask;
	if (newmask != oldmask)
		xkl_engine_select_input(engine, win, newmask);
}

void
xkl_engine_try_call_state_func(XklEngine * engine,
			       XklStateChange change_type,
			       XklState * old_state)
{
	gint group = engine->priv->curr_state.group;
	gboolean restore = old_state->group == group;

	xkl_debug(150,
		  "change_type: %d, group: %d, secondary_group_mask: %X, allowsecondary: %d\n",
		  change_type, group, engine->priv->secondary_groups_mask,
		  xkl_engine_is_one_switch_to_secondary_group_allowed
		  (engine));

	if (change_type == GROUP_CHANGED) {
		if (!restore) {
			if ((engine->priv->
			     secondary_groups_mask & (1 << group)) != 0
			    &&
			    !xkl_engine_is_one_switch_to_secondary_group_allowed
			    (engine)) {
				xkl_debug(150, "secondary -> go next\n");
				group = xkl_engine_get_next_group(engine);
				xkl_engine_lock_group(engine, group);
				return;	/* we do not need to revalidate */
			}
		}
		xkl_engine_one_switch_to_secondary_group_performed(engine);
	}
// TODO
#if 0
	if (xkl_state_callback != NULL) {

		(*xkl_state_callback) (change_type,
				       xkl_curr_state.group, restore,
				       xkl_state_callback_data);
	}
#endif
}

void
xkl_engine_ensure_vtable_inited(XklEngine * engine)
{
	char *p;
	if (engine->priv->backend_id == NULL) {
		xkl_debug(0, "ERROR: XKL VTable is NOT initialized.\n");
		/* force the crash! */
		p = NULL;
		*p = '\0';
	}
}

const gchar *
xkl_engine_get_backend_name(XklEngine * engine)
{
	return engine->priv->backend_id;
}

guint
xkl_engine_get_features(XklEngine * engine)
{
	return engine->priv->features;
}

void
xkl_engine_reset_all_info(XklEngine * engine, const gchar reason[])
{
	xkl_debug(150, "Resetting all the cached info, reason: [%s]\n",
		  reason);
	xkl_engine_ensure_vtable_inited(engine);
	if (!xkl_engine_vcall(engine, if_cached_info_equals_actual)
	    (engine)) {
		xkl_engine_vcall(engine, free_all_info) (engine);
		xkl_engine_vcall(engine, load_all_info) (engine);
	} else
		xkl_debug(100,
			  "NOT Resetting the cache: same configuration\n");
}

/**
 * Calling through vtable
 */
const gchar **
xkl_engine_groups_get_names(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return (*engine->priv->get_groups_names) (engine);
}

guint
xkl_engine_get_num_groups(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return (*engine->priv->get_num_groups) (engine);
}

void
xkl_engine_lock_group(XklEngine * engine, int group)
{
	xkl_engine_ensure_vtable_inited(engine);
	xkl_engine_vcall(engine, lock_group) (engine, group);
}

gint
xkl_engine_pause_listen(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, pause_listen) (engine);
}

gint
xkl_engine_resume_listen(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	xkl_debug(150, "listenerType: %x\n", engine->priv->listener_type);
	if (xkl_engine_vcall(engine, resume_listen) (engine))
		return 1;

	xkl_engine_select_input_merging(engine, engine->priv->root_window,
					SubstructureNotifyMask |
					PropertyChangeMask);

// TODO
#if 0
	(*xkl_vtable->get_server_state_func) (&xkl_curr_state);
#endif
	return 0;
}

gboolean
xkl_engine_load_all_info(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return xkl_engine_vcall(engine, load_all_info) (engine);
}

void
xkl_engine_free_all_info(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	xkl_engine_vcall(engine, free_all_info) (engine);
}

guint
xkl_engine_get_max_num_groups(XklEngine * engine)
{
	xkl_engine_ensure_vtable_inited(engine);
	return (*engine->priv->get_max_num_groups) (engine);
}

XklEngine *
xkl_get_the_engine()
{
	return the_engine;
}
