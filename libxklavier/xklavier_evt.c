#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"

gint
xkl_events_filter(XEvent * xev)
{
	XAnyEvent *pe = (XAnyEvent *) xev;
	xkl_debug(400,
		  "**> Filtering event %d of type %d from window %d\n",
		  pe->serial, pe->type, pe->window);
	xkl_ensure_vtable_inited();
	if (!xkl_vtable->process_x_event_func(xev))
		switch (xev->type) {	/* core events */
		case FocusIn:
			xkl_process_focus_in_evt(&xev->xfocus);
			break;
		case FocusOut:
			xkl_process_focus_out_evt(&xev->xfocus);
			break;
		case PropertyNotify:
			xkl_process_property_evt(&xev->xproperty);
			break;
		case CreateNotify:
			xkl_process_create_window_evt(&xev->xcreatewindow);
			break;
		case DestroyNotify:
			xkl_debug(150,
				  "Window " WINID_FORMAT " destroyed\n",
				  xev->xdestroywindow.window);
			break;
		case UnmapNotify:
		case MapNotify:
		case GravityNotify:
		case ReparentNotify:
			xkl_debug(200, "%s\n",
				  xkl_event_get_name(xev->type));
			break;	/* Ignore these events */
		case MappingNotify:
			xkl_debug(200, "%s\n",
				  xkl_event_get_name(xev->type));
			xkl_reset_all_info("X event: MappingNotify");
			break;
		default:
			{
				xkl_debug(200, "Unknown event %d [%s]\n",
					  xev->type,
					  xkl_event_get_name(xev->type));
				return 1;
			}
		}
	xkl_debug(400, "Filtered event %d of type %d from window %d **>\n",
		  pe->serial, pe->type, pe->window);
	return 1;
}

/**
 * FocusIn handler
 */
void
xkl_process_focus_in_evt(XFocusChangeEvent * fev)
{
	Window win;
	Window toplevel_win;
	XklState selected_window_state;

	if (!(xkl_listener_type & XKLL_MANAGE_WINDOW_STATES))
		return;

	win = fev->window;

	switch (fev->mode) {
	case NotifyNormal:
	case NotifyWhileGrabbed:
		break;
	default:
		xkl_debug(160,
			  "Window " WINID_FORMAT
			  " has got focus during special action %d\n", win,
			  fev->mode);
		return;
	}

	xkl_debug(150, "Window " WINID_FORMAT ", '%s' has got focus\n",
		  win, xkl_window_get_debug_title(win));

	if (!xkl_toplevel_window_find(win, &toplevel_win)) {
		return;
	}

	xkl_debug(150, "Appwin " WINID_FORMAT ", '%s' has got focus\n",
		  toplevel_win, xkl_window_get_debug_title(toplevel_win));

	if (xkl_state_get(toplevel_win, &selected_window_state)) {
		if (xkl_current_client != toplevel_win) {
			gboolean old_win_transparent, new_win_transparent;
			XklState tmp_state;

			old_win_transparent =
			    xkl_toplevel_window_is_transparent
			    (xkl_current_client);
			if (old_win_transparent)
				xkl_debug(150,
					  "Leaving transparent window\n");

      /**
       * Reload the current state from the current window. 
       * Do not do it for transparent window - we keep the state from 
       * the _previous_ window.
       */
			if (!old_win_transparent &&
			    xkl_state_get(xkl_current_client, &tmp_state))
			{
				xkl_current_state_update(tmp_state.group,
							 tmp_state.
							 indicators,
							 "Loading current (previous) state from the current (previous) window");
			}

			xkl_current_client = toplevel_win;
			xkl_debug(150,
				  "CurClient:changed to " WINID_FORMAT
				  ", '%s'\n", xkl_current_client,
				  xkl_window_get_debug_title
				  (xkl_current_client));

			new_win_transparent =
			    xkl_toplevel_window_is_transparent
			    (toplevel_win);
			if (new_win_transparent)
				xkl_debug(150,
					  "Entering transparent window\n");

			if (xkl_is_group_per_toplevel_window() ==
			    !new_win_transparent) {
				/* We skip restoration only if we return to the same app window */
				gboolean do_skip = FALSE;
				if (xkl_skip_one_restore) {
					xkl_skip_one_restore = FALSE;
					if (toplevel_win ==
					    xkl_toplevel_window_prev)
						do_skip = TRUE;
				}

				if (do_skip) {
					xkl_debug(150,
						  "Skipping one restore as requested - instead, "
						  "saving the current group into the window state\n");
					xkl_toplevel_window_save_state
					    (toplevel_win,
					     &xkl_current_state);
				} else {
					if (xkl_current_state.group !=
					    selected_window_state.group) {
						xkl_debug(150,
							  "Restoring the group from %d to %d after gaining focus\n",
							  xkl_current_state.
							  group,
							  selected_window_state.
							  group);
	    /**
             *  For fast mouse movements - the state is probably not updated yet
             *  (because of the group change notification being late).
             *  so we'll enforce the update. But this should only happen in GPA mode
             */
						xkl_current_state_update
						    (selected_window_state.
						     group,
						     selected_window_state.
						     indicators,
						     "Enforcing fast update of the current state");
						xkl_group_lock
						    (selected_window_state.
						     group);
					} else {
						xkl_debug(150,
							  "Both old and new focused window "
							  "have group %d so no point restoring it\n",
							  selected_window_state.
							  group);
						xkl_one_switch_to_secondary_group_performed
						    ();
					}
				}

				if ((xkl_vtable->
				     features & XKLF_CAN_TOGGLE_INDICATORS)
				    && xkl_get_indicators_handling()) {
					xkl_debug(150,
						  "Restoring the indicators from %X to %X after gaining focus\n",
						  xkl_current_state.
						  indicators,
						  selected_window_state.
						  indicators);
					xkl_ensure_vtable_inited();
					(*xkl_vtable->
					 indicators_set_func)
			       (&selected_window_state);
				} else
					xkl_debug(150,
						  "Not restoring the indicators %X after gaining focus: indicator handling is not enabled\n",
						  xkl_current_state.
						  indicators);
			} else
				xkl_debug(150,
					  "Not restoring the group %d after gaining focus: global layout (xor transparent window)\n",
					  xkl_current_state.group);
		} else
			xkl_debug(150,
				  "Same app window - just do nothing\n");
	} else {
		xkl_debug(150, "But it does not have xklavier_state\n");
		if (xkl_window_has_wm_state(win)) {
			xkl_debug(150,
				  "But it does have wm_state so we'll add it\n");
			xkl_current_client = toplevel_win;
			xkl_debug(150,
				  "CurClient:changed to " WINID_FORMAT
				  ", '%s'\n", xkl_current_client,
				  xkl_window_get_debug_title
				  (xkl_current_client));
			xkl_toplevel_window_add(xkl_current_client,
						(Window) NULL, FALSE,
						&xkl_current_state);
		} else
			xkl_debug(150,
				  "And it does have wm_state either\n");
	}
}

/** 
 * FocusOut handler
 */
void
xkl_process_focus_out_evt(XFocusChangeEvent * fev)
{
	if (!(xkl_listener_type & XKLL_MANAGE_WINDOW_STATES))
		return;

	if (fev->mode != NotifyNormal) {
		xkl_debug(200,
			  "Window " WINID_FORMAT
			  " has lost focus during special action %d\n",
			  fev->window, fev->mode);
		return;
	}

	xkl_debug(160, "Window " WINID_FORMAT ", '%s' has lost focus\n",
		  fev->window, xkl_window_get_debug_title(fev->window));

	if (xkl_window_is_transparent(fev->window)) {
		xkl_debug(150, "Leaving transparent window!\n");
/** 
 * If we are leaving the transparent window - we skip the restore operation.
 * This is useful for secondary groups switching from the transparent control 
 * window.
 */
		xkl_skip_one_restore = TRUE;
	} else {
		Window p;
		if (xkl_toplevel_window_find(fev->window, &p))
			xkl_toplevel_window_prev = p;
	}
}

/**
 * PropertyChange handler
 * Interested in :
 *  + for XKLL_MANAGE_WINDOW_STATES
 *    - WM_STATE property for all windows
 *    - Configuration property of the root window
 */
void
xkl_process_property_evt(XPropertyEvent * pev)
{
	if (400 <= xkl_debug_level) {
		char *atom_name = XGetAtomName(xkl_display, pev->atom);
		if (atom_name != NULL) {
			xkl_debug(400,
				  "The property '%s' changed for "
				  WINID_FORMAT "\n", atom_name,
				  pev->window);
			XFree(atom_name);
		} else {
			xkl_debug(200,
				  "Some magic property changed for "
				  WINID_FORMAT "\n", pev->window);
		}
	}

	if (xkl_listener_type & XKLL_MANAGE_WINDOW_STATES) {
		if (pev->atom == xkl_atoms[WM_STATE]) {
			gboolean has_xkl_state =
			    xkl_state_get(pev->window, NULL);

			if (pev->state == PropertyNewValue) {
				xkl_debug(160,
					  "New value of WM_STATE on window "
					  WINID_FORMAT "\n", pev->window);
				if (!has_xkl_state) {	/* Is this event the first or not? */
					xkl_toplevel_window_add(pev->
								window,
								(Window)
								NULL,
								FALSE,
								&xkl_current_state);
				}
			} else {	/* ev->xproperty.state == PropertyDelete, either client or WM can remove it, ICCCM 4.1.3.1 */
				xkl_debug(160,
					  "Something (%d) happened to WM_STATE of window 0x%x\n",
					  pev->state, pev->window);
				xkl_select_input_merging(pev->window,
							 PropertyChangeMask);
				if (has_xkl_state) {
					xkl_state_delete(pev->window);
				}
			}
		} else
		    if (pev->atom == xkl_vtable->base_config_atom
			&& pev->window == xkl_root_window) {
			if (pev->state == PropertyNewValue) {
				/* If root window got new *_NAMES_PROP_ATOM -
				   it most probably means new keyboard config is loaded by somebody */
				xkl_reset_all_info
				    ("New value of *_NAMES_PROP_ATOM on root window");
			}
		}
	}			/* XKLL_MANAGE_WINDOW_STATES */
}

/**
 * CreateNotify handler. Just interested in properties and focus events...
 */
void
xkl_process_create_window_evt(XCreateWindowEvent * cev)
{
	if (!(xkl_listener_type & XKLL_MANAGE_WINDOW_STATES))
		return;

	xkl_debug(200,
		  "Under-root window " WINID_FORMAT
		  "/%s (%d,%d,%d x %d) is created\n", cev->window,
		  xkl_window_get_debug_title(cev->window), cev->x, cev->y,
		  cev->width, cev->height);

	if (!cev->override_redirect) {
/* ICCCM 4.1.6: override-redirect is NOT private to
* client (and must not be changed - am I right?) 
* We really need only PropertyChangeMask on this window but even in the case of
* local server we can lose PropertyNotify events (the trip time for this CreateNotify
* event + SelectInput request is not zero) and we definitely will (my system DO)
* lose FocusIn/Out events after the following call of PropertyNotifyHandler.
* So I just decided to purify this extra FocusChangeMask in the FocusIn/OutHandler. */
		xkl_select_input_merging(cev->window,
					 PropertyChangeMask |
					 FocusChangeMask);

		if (xkl_window_has_wm_state(cev->window)) {
			xkl_debug(200,
				  "Just created window already has WM_STATE - so I'll add it");
			xkl_toplevel_window_add(cev->window, (Window) NULL,
						FALSE, &xkl_current_state);
		}
	}
}

/**
 * Just error handler - sometimes we get BadWindow error for already gone 
 * windows, so we'll just ignore
 */
void
xkl_process_error(Display * dpy, XErrorEvent * evt)
{
	xkl_last_error_code = evt->error_code;
	switch (xkl_last_error_code) {
	case BadWindow:
	case BadAccess:
		{
			/* in most cases this means we are late:) */
			xkl_debug(200,
				  "ERROR: %p, " WINID_FORMAT
				  ", %d, %d, %d\n", dpy,
				  (unsigned long) evt->resourceid,
				  (int) evt->error_code,
				  (int) evt->request_code,
				  (int) evt->minor_code);
			break;
		}
	default:
		(*xkl_default_error_handler) (dpy, evt);
	}
}

/**
 * Some common functionality for Xkb handler
 */
void
xkl_process_state_modification(XklStateChange change_type,
			       gint grp, guint inds, gboolean set_inds)
{
	Window focused, focused_toplevel;
	XklState old_state;
	gint revert;
	gboolean have_old_state = TRUE;
	gboolean set_group = change_type == GROUP_CHANGED;

	XGetInputFocus(xkl_display, &focused, &revert);

	if ((focused == None) || (focused == PointerRoot)) {
		xkl_debug(160, "Something with focus: " WINID_FORMAT "\n",
			  focused);
		return;
	}

  /** 
   * Only if we manage states - otherwise xkl_current_client does not make sense 
   */
	if (!xkl_toplevel_window_find(focused, &focused_toplevel) &&
	    xkl_listener_type & XKLL_MANAGE_WINDOW_STATES)
		focused_toplevel = xkl_current_client;	/* what else can I do */

	xkl_debug(150, "Focused window: " WINID_FORMAT ", '%s'\n",
		  focused_toplevel,
		  xkl_window_get_debug_title(focused_toplevel));
	if (xkl_listener_type & XKLL_MANAGE_WINDOW_STATES) {
		xkl_debug(150, "CurClient: " WINID_FORMAT ", '%s'\n",
			  xkl_current_client,
			  xkl_window_get_debug_title(xkl_current_client));

		if (focused_toplevel != xkl_current_client) {
      /**
       * If not state - we got the new window
       */
			if (!xkl_toplevel_window_get_state
			    (focused_toplevel, &old_state)) {
				xkl_current_state_update(grp, inds,
							 "Updating the state from new focused window");
				if (xkl_listener_type &
				    XKLL_MANAGE_WINDOW_STATES)
					xkl_toplevel_window_add
					    (focused_toplevel,
					     (Window) NULL, FALSE,
					     &xkl_current_state);
			}
      /**
       * There is state - just get the state from the window
       */
			else {
				grp = old_state.group;
				inds = old_state.indicators;
			}
			xkl_current_client = focused_toplevel;
			xkl_debug(160,
				  "CurClient:changed to " WINID_FORMAT
				  ", '%s'\n", xkl_current_client,
				  xkl_window_get_debug_title
				  (xkl_current_client));
		}
		/* If the window already has this this state - we are just restoring it!
		   (see the second parameter of stateCallback */
		have_old_state =
		    xkl_toplevel_window_get_state(xkl_current_client,
						  &old_state);
	} else {		/* just tracking the stuff, no smart things */

		xkl_debug(160,
			  "Just updating the current state in the tracking mode\n");
		memcpy(&old_state, &xkl_current_state, sizeof(XklState));
	}

	if (set_group || have_old_state) {
		xkl_current_state_update(set_group ? grp : old_state.group,
					 set_inds ? inds : old_state.
					 indicators,
					 "Restoring the state from the window");
	}

	if (have_old_state)
		xkl_try_call_state_func(change_type, &old_state);

	if (xkl_listener_type & XKLL_MANAGE_WINDOW_STATES)
		xkl_toplevel_window_save_state(xkl_current_client,
					       &xkl_current_state);
}
