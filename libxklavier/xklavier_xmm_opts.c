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

XmmSwitchOption all_switch_options[] = 
{
  {
    "ralt_toggle",
    { { XK_Alt_R, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "lalt_toggle",
    { { XK_Alt_L, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "caps_toggle",
    { { XK_Caps_Lock, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "shift_caps_toggle",
    { { XK_Caps_Lock, ShiftMask }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "shifts_toggle",
    { { XK_Shift_R, ShiftMask },
      { XK_Shift_L, ShiftMask }, { XK_VoidSymbol } }, { 1, -1 }
  },
  {
    "alts_toggle",
    { { XK_Alt_R, Mod1Mask },
      { XK_Alt_L, Mod1Mask }, { XK_VoidSymbol } }, { 1, -1 }
  },
  {
    "ctrls_toggle",
    { { XK_Control_R, ControlMask },
      { XK_Control_L, ControlMask }, { XK_VoidSymbol } }, { 1, -1 }
  },
  {
    "ctrl_shift_toggle",
    { { XK_Control_R, ShiftMask },
      { XK_Control_L, ShiftMask },
      { XK_Shift_R, ControlMask },
      { XK_Shift_L, ControlMask }, { XK_VoidSymbol } }, { 1, -1, 1, -1 }
  },
  {
    "ctrl_alt_toggle",
    { { XK_Control_R, Mod1Mask }, 
      { XK_Control_L, Mod1Mask },
      { XK_Alt_R, ControlMask },
      { XK_Alt_L, ControlMask }, { XK_VoidSymbol } }, { 1, -1, 1, -1 }
  },
  {
    "alt_shift_toggle",
    { { XK_Shift_R, Mod1Mask }, 
      { XK_Shift_L, Mod1Mask },
      { XK_Alt_R, ShiftMask },
      { XK_Alt_L, ShiftMask }, { XK_VoidSymbol } }, { 1, -1, 1, -1 }
  },
  {
    "menu_toggle",
    { { XK_Menu, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "lwin_toggle",
    { { XK_Super_L, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "rwin_toggle",
    { { XK_Super_R, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "lshift_toggle",
    { { XK_Shift_L, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "rshift_toggle",
    { { XK_Shift_R, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "lctrl_toggle",
    { { XK_Control_L, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    "rctrl_toggle",
    { { XK_Control_R, 0 }, { XK_VoidSymbol } }, { 1 }
  },
  {
    NULL,
    { { 0, 0 }, { XK_VoidSymbol } }, { 1 }
  }
};
