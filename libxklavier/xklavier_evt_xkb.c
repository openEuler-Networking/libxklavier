#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

/*
 * XKB event handler
 */
gint
xkl_xkb_process_x_event(XklEngine * engine, XEvent * xev)
{
#ifdef XKB_HEADERS_PRESENT
	gint i;
	guint bit;
	guint inds;
	XkbEvent *kev = (XkbEvent *) xev;

	if (xev->type != xkl_engine_backend(engine, XklXkb, event_type))
		return 0;

	if (!(xkl_engine_priv(engine, listener_type) &
	      (XKLL_MANAGE_WINDOW_STATES | XKLL_TRACK_KEYBOARD_STATE)))
		return 0;

	xkl_debug(150, "Xkb event detected\n");

	switch (kev->any.xkb_type) {
		/*
		 * Group is changed!
		 */
	case XkbStateNotify:
#define GROUP_CHANGE_MASK \
    ( XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask )

		xkl_debug(150,
			  "XkbStateNotify detected, changes: %X/(mask %X), new group %d\n",
			  kev->state.changed, GROUP_CHANGE_MASK,
			  kev->state.locked_group);

		if (kev->state.changed & GROUP_CHANGE_MASK)
			xkl_engine_process_state_modification(engine,
							      GROUP_CHANGED,
							      kev->state.
							      locked_group,
							      0, FALSE);
		else {		/* ...not interested... */

			xkl_debug(200,
				  "This type of state notification is not regarding groups\n");
			if (kev->state.locked_group !=
			    xkl_engine_priv(engine, curr_state).group)
				xkl_debug(0,
					  "ATTENTION! Currently cached group %d is not equal to the current group from the event: %d\n!",
					  xkl_engine_priv(engine,
							  curr_state).
					  group, kev->state.locked_group);
		}

		break;

		/*
		 * Indicators are changed!
		 */
	case XkbIndicatorStateNotify:

		xkl_debug(150, "XkbIndicatorStateNotify\n");

		inds = xkl_engine_priv(engine, curr_state).indicators;

		ForPhysIndicators(i,
				  bit) if (kev->indicators.changed & bit) {
			if (kev->indicators.state & bit)
				inds |= bit;
			else
				inds &= ~bit;
		}

		xkl_engine_process_state_modification(engine,
						      INDICATORS_CHANGED,
						      0, inds, TRUE);
		break;

		/*
		 * The configuration is changed!
		 */
	case XkbIndicatorMapNotify:
	case XkbControlsNotify:
	case XkbNamesNotify:
#if 0
		/* not really fair - but still better than flooding... */
		XklDebug(200,
			 "warning: configuration event %s is not actually processed\n",
			 _XklXkbGetXkbEventName(kev->any.xkb_type));
		break;
#endif
	case XkbNewKeyboardNotify:
		xkl_debug(150, "%s\n",
			  xkl_xkb_event_get_name(kev->any.xkb_type));
		xkl_engine_reset_all_info(engine,
					  "XKB event: XkbNewKeyboardNotify");
		break;

		/*
		 * ...Not interested...
		 */
	default:
		xkl_debug(150, "Unknown XKB event %d [%s]\n",
			  kev->any.xkb_type,
			  xkl_xkb_event_get_name(kev->any.xkb_type));
		return 0;
	}
	return 1;
#else
	return 0;
#endif
}

void
xkl_xkb_set_indicators(XklEngine * engine, const XklState * window_state)
{
#ifdef XKB_HEADERS_PRESENT
	int i;
	unsigned bit;

	XkbDescPtr cached =
	    xkl_engine_backend(engine, XklXkb, cached_desc);
	ForPhysIndicators(i, bit) if (cached->names->indicators[i] != None) {
		gboolean status;
		status = xkl_xkb_set_indicator(engine, i,
					       (window_state->
						indicators & bit) != 0);
		xkl_debug(150, "Set indicator \"%s\"/%d to %d: %d\n",
			  xkl_engine_backend(engine, XklXkb,
					     indicator_names)[i],
			  cached->names->indicators[i],
			  window_state->indicators & bit, status);
	}
#endif
}
