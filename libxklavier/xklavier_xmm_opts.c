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

XmmSwitchOption allSwitchOptions[] = 
{
  {
    "ralt_toggle", 1,
    { { XK_Alt_R, 0 } }, { 1 }
  },
  {
    "lalt_toggle", 1,
    { { XK_Alt_L, 0 } }, { 1 }
  },
  {
    "caps_toggle", 1,
    { { XK_Caps_Lock, 0 } }, { 1 }
  },
  {
    "shift_caps_toggle", 1,
    { { XK_Caps_Lock, ShiftMask } }, { 1 }
  },
  {
    "shifts_toggle", 2,
    { { XK_Shift_R, ShiftMask },
      { XK_Shift_L, ShiftMask } }, { 1, -1 }
  },
  {
    "alts_toggle", 2, 
    { { XK_Alt_R, Mod1Mask },
      { XK_Alt_L, Mod1Mask } }, { 1, -1 }
  },
  {
    "ctrls_toggle", 2, 
    { { XK_Control_R, ControlMask },
      { XK_Control_L, ControlMask } }, { 1, -1 }
  },
  {
    "ctrl_shift_toggle", 4, 
    { { XK_Control_R, ShiftMask },
      { XK_Control_L, ShiftMask },
      { XK_Shift_R, ControlMask },
      { XK_Shift_L, ControlMask } }, { 1, -1, 1, -1 }
  },
  {
    "ctrl_alt_toggle", 4, 
    { { XK_Control_R, Mod1Mask }, 
      { XK_Control_L, Mod1Mask },
      { XK_Alt_R, ControlMask },
      { XK_Alt_L, ControlMask } }, { 1, -1, 1, -1 }
  },
  {
    "alt_shift_toggle", 4, 
    { { XK_Shift_R, Mod1Mask }, 
      { XK_Shift_L, Mod1Mask },
      { XK_Alt_R, ShiftMask },
      { XK_Alt_L, ShiftMask } }, { 1, -1, 1, -1 }
  },
  {
    "menu_toggle", 1, 
    { { XK_Menu, 0 } }, { 1 }
  },
  {
    "lwin_toggle", 1, 
    { { XK_Super_L, 0 } }, { 1 }
  },
  {
    "rwin_toggle", 1, 
    { { XK_Super_R, 0 } }, { 1 }
  },
  {
    "lshift_toggle", 1, 
    { { XK_Shift_L, 0 } }, { 1 }
  },
  {
    "rshift_toggle", 1, 
    { { XK_Shift_R, 0 } }, { 1 }
  },
  {
    "lctrl_toggle", 1, 
    { { XK_Control_L, 0 } }, { 1 }
  },
  {
    "rctrl_toggle", 1, 
    { { XK_Control_R, 0 } }, { 1 }
  },
  {
    NULL, 1,
    { { 0, 0 } }, { 1 }
  }
};
