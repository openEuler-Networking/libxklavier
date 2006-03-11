#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

static gint
xkl_xmm_process_keypress_event(XklEngine * engine, XKeyPressedEvent * kpe)
{
	if (xkl_engine_priv(engine, listener_type) & XKLL_MANAGE_LAYOUTS) {
		xkl_debug(200, "Processing the KeyPress event\n");
		gint current_shortcut = 0;
		const XmmSwitchOption *sop =
		    xkl_xmm_find_switch_option(engine, kpe->keycode,
					       kpe->state,
					       &current_shortcut);
		if (sop != NULL) {
			xkl_debug(150, "It is THE shortcut\n");
			XklState state;
			xkl_xmm_get_server_state(engine, &state);
			if (state.group != -1) {
				gint new_group =
				    (state.group +
				     sop->
				     shortcut_steps[current_shortcut]) %
				    g_strv_length(xkl_engine_backend
						  (engine, XklXmm,
						   current_config).
						  layouts);
				xkl_debug(150,
					  "Setting new xmm group %d\n",
					  new_group);
				xkl_xmm_lock_group(engine, new_group);
				return 1;
			}
		}
	}
	return 0;
}

static gint
xkl_xmm_process_property_event(XklEngine * engine, XPropertyEvent * kpe)
{
	Atom state_atom = xkl_engine_backend(engine, XklXmm, state_atom);
	xkl_debug(200, "Processing the PropertyNotify event: %d/%d\n",
		  kpe->atom, state_atom);
  /*
   * Group is changed!
   */
	if (kpe->atom == state_atom) {
		XklState state;
		xkl_xmm_get_server_state(engine, &state);

		if (xkl_engine_priv(engine, listener_type) &
		    XKLL_MANAGE_LAYOUTS) {
			xkl_debug(150,
				  "Current group from the root window property %d\n",
				  state.group);
			xkl_xmm_shortcuts_ungrab(engine);
			xkl_xmm_actualize_group(engine, state.group);
			xkl_xmm_shortcuts_grab(engine);
			return 1;
		}

		if (xkl_engine_priv(engine, listener_type) &
		    (XKLL_MANAGE_WINDOW_STATES |
		     XKLL_TRACK_KEYBOARD_STATE)) {
			xkl_debug(150,
				  "XMM state changed, new 'group' %d\n",
				  state.group);

			xkl_engine_process_state_modification(engine,
							      GROUP_CHANGED,
							      state.group,
							      0, False);
		}
	} else
  /*
   * Configuration is changed!
   */
	if (kpe->atom == xkl_engine_priv(engine, base_config_atom)) {
		xkl_engine_reset_all_info(engine,
					  "base config atom changed");
	}

	return 0;
}

/*
 * XMM event handler
 */
gint
xkl_xmm_process_x_event(XklEngine * engine, XEvent * xev)
{
	switch (xev->type) {
	case KeyPress:
		return xkl_xmm_process_keypress_event(engine,
						      (XKeyPressedEvent *)
						      xev);
	case PropertyNotify:
		return xkl_xmm_process_property_event(engine,
						      (XPropertyEvent *)
						      xev);
	}
	return 0;
}

/*
 * We have to find which of Shift/Lock/Control/ModX masks
 * belong to Caps/Num/Scroll lock
 */
static void
xkl_xmm_init_xmm_indicators_map(XklEngine * engine,
				guint * p_caps_lock_mask,
				guint * p_num_lock_mask,
				guint * p_scroll_lock_mask)
{
	XModifierKeymap *xmkm = NULL;
	KeyCode *kcmap, nlkc, clkc, slkc;
	int m, k, mask;

	Display *display = xkl_engine_get_display(engine);
	xmkm = XGetModifierMapping(display);
	if (xmkm) {
		clkc = XKeysymToKeycode(display, XK_Num_Lock);
		nlkc = XKeysymToKeycode(display, XK_Caps_Lock);
		slkc = XKeysymToKeycode(display, XK_Scroll_Lock);

		kcmap = xmkm->modifiermap;
		mask = 1;
		for (m = 8; --m >= 0; mask <<= 1)
			for (k = xmkm->max_keypermod; --k >= 0; kcmap++) {
				if (*kcmap == clkc)
					*p_caps_lock_mask = mask;
				if (*kcmap == slkc)
					*p_scroll_lock_mask = mask;
				if (*kcmap == nlkc)
					*p_num_lock_mask = mask;
			}
		XFreeModifiermap(xmkm);
	}
}

void
xkl_xmm_grab_ignoring_indicators(XklEngine * engine, gint keycode,
				 guint modifiers)
{
	guint caps_lock_mask = 0, num_lock_mask = 0, scroll_lock_mask = 0;

	xkl_xmm_init_xmm_indicators_map(engine, &caps_lock_mask,
					&num_lock_mask, &scroll_lock_mask);

#define GRAB(mods) \
  xkl_engine_grab_key(engine, keycode, modifiers|(mods))

	GRAB(0);
	GRAB(caps_lock_mask);
	GRAB(num_lock_mask);
	GRAB(scroll_lock_mask);
	GRAB(caps_lock_mask | num_lock_mask);
	GRAB(caps_lock_mask | scroll_lock_mask);
	GRAB(num_lock_mask | scroll_lock_mask);
	GRAB(caps_lock_mask | num_lock_mask | scroll_lock_mask);
#undef GRAB
}

void
xkl_xmm_ungrab_ignoring_indicators(XklEngine * engine, gint keycode,
				   guint modifiers)
{
	guint caps_lock_mask = 0, num_lock_mask = 0, scroll_lock_mask = 0;

	xkl_xmm_init_xmm_indicators_map(engine, &caps_lock_mask,
					&num_lock_mask, &scroll_lock_mask);

#define UNGRAB(mods) \
  xkl_engine_ungrab_key(engine, keycode, modifiers|(mods))

	UNGRAB(0);
	UNGRAB(caps_lock_mask);
	UNGRAB(num_lock_mask);
	UNGRAB(scroll_lock_mask);
	UNGRAB(caps_lock_mask | num_lock_mask);
	UNGRAB(caps_lock_mask | scroll_lock_mask);
	UNGRAB(num_lock_mask | scroll_lock_mask);
	UNGRAB(caps_lock_mask | num_lock_mask | scroll_lock_mask);
#undef UNGRAB
}
