#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

#include "xklavier_private.h"
#include "xklavier_private_xkb.h"

XkbDescPtr _xklXkb;

char *_xklIndicatorNames[XkbNumIndicators];

Atom _xklIndicatorAtoms[XkbNumIndicators];

unsigned _xklPhysIndicatorsMask;

int _xklXkbEventType, _xklXkbError;

static char *groupNames[XkbNumKbdGroups];

const char **XklGetGroupNames( void )
{
  return ( const char ** ) groupNames;
}

int XklInit( Display * a_dpy )
{
  int opcode;
  int scr;

  _xklDefaultErrHandler =
    XSetErrorHandler( ( XErrorHandler ) _XklErrHandler );

  /* Lets begin */
  _xklXkbExtPresent = XkbQueryExtension( _xklDpy = a_dpy,
                                         &opcode, &_xklXkbEventType,
                                         &_xklXkbError, NULL, NULL );
  if( !_xklXkbExtPresent )
  {
    return -1;
  }

  scr = DefaultScreen( _xklDpy );
  _xklRootWindow = RootWindow( _xklDpy, scr );
  XklDebug( 160,
            "xkbEvenType: %X, xkbError: %X, display: %p, root: " WINID_FORMAT
            "\n", _xklXkbEventType, _xklXkbError, _xklDpy, _xklRootWindow );

  _xklAtoms[WM_NAME] = XInternAtom( _xklDpy, "WM_NAME", False );
  _xklAtoms[WM_STATE] = XInternAtom( _xklDpy, "WM_STATE", False );
  _xklAtoms[XKLAVIER_STATE] = XInternAtom( _xklDpy, "XKLAVIER_STATE", False );
  _xklAtoms[XKLAVIER_TRANSPARENT] =
    XInternAtom( _xklDpy, "XKLAVIER_TRANSPARENT", False );
  _xklAtoms[XKB_RF_NAMES_PROP_ATOM] =
    XInternAtom( _xklDpy, _XKB_RF_NAMES_PROP_ATOM, False );
  _xklAtoms[XKB_RF_NAMES_PROP_ATOM_BACKUP] =
    XInternAtom( _xklDpy, "_XKB_RULES_NAMES_BACKUP", False );

  _xklAllowSecondaryGroupOnce = False;
  _xklSkipOneRestore = False;
  _xklDefaultGroup = -1;
  _xklSecondaryGroupsMask = 0L;
  _xklPrevAppWindow = 0;

  return _XklLoadAllInfo(  )? 0 : _xklLastErrorCode;
}

int XklPauseListen(  )
{
  XkbSelectEvents( _xklDpy, XkbUseCoreKbd, XkbAllEventsMask, 0 );
//  XkbSelectEventDetails( _xklDpy,
  //                       XkbUseCoreKbd,
  //                     XkbStateNotify,
  //                   0,
  //                 0 );

  //!!_XklSelectInput( _xklRootWindow, 0 );
  return 0;
}

int XklResumeListen(  )
{
  /* What events we want */
#define XKB_EVT_MASK \
         (XkbStateNotifyMask| \
          XkbNamesNotifyMask| \
          XkbControlsNotifyMask| \
          XkbIndicatorStateNotifyMask| \
          XkbIndicatorMapNotifyMask| \
          XkbNewKeyboardNotifyMask)

  XkbSelectEvents( _xklDpy, XkbUseCoreKbd, XKB_EVT_MASK, XKB_EVT_MASK );

#define XKB_STATE_EVT_DTL_MASK \
         (XkbGroupStateMask)

  XkbSelectEventDetails( _xklDpy,
                         XkbUseCoreKbd,
                         XkbStateNotify,
                         XKB_STATE_EVT_DTL_MASK, XKB_STATE_EVT_DTL_MASK );

#define XKB_NAMES_EVT_DTL_MASK \
         (XkbGroupNamesMask|XkbIndicatorNamesMask)

  XkbSelectEventDetails( _xklDpy,
                         XkbUseCoreKbd,
                         XkbNamesNotify,
                         XKB_NAMES_EVT_DTL_MASK, XKB_NAMES_EVT_DTL_MASK );

  _XklSelectInputMerging( _xklRootWindow,
                          SubstructureNotifyMask | PropertyChangeMask );
  _XklGetRealState( &_xklCurState );
  return 0;
}

unsigned XklGetNumGroups(  )
{
  return _xklXkb->ctrls->num_groups;
}

int XklGetNextGroup(  )
{
  return ( _xklCurState.group + 1 ) % _xklXkb->ctrls->num_groups;
}

int XklGetPrevGroup(  )
{
  int n = _xklXkb->ctrls->num_groups;
  return ( _xklCurState.group + n - 1 ) % n;
}

#define KBD_MASK \
    ( 0 )
#define CTRLS_MASK \
  ( XkbSlowKeysMask )
#define NAMES_MASK \
  ( XkbGroupNamesMask | XkbIndicatorNamesMask )

void _XklFreeAllInfo(  )
{
  if( _xklXkb != NULL )
  {
    int i;
    char **groupName = groupNames;
    for( i = _xklXkb->ctrls->num_groups; --i >= 0; groupName++ )
      if( *groupName )
        XFree( *groupName );
    XkbFreeKeyboard( _xklXkb, XkbAllComponentsMask, True );
    _xklXkb = NULL;
  }
}

/**
 * Load some XKB parameters
 */
Bool _XklLoadAllInfo(  )
{
  int i;
  unsigned bit;
  Atom *gna;
  char **groupName;

  _xklXkb = XkbGetMap( _xklDpy, KBD_MASK, XkbUseCoreKbd );
  if( _xklXkb == NULL )
  {
    _xklLastErrorMsg = "Could not load keyboard";
    return False;
  }

  _xklLastErrorCode = XkbGetControls( _xklDpy, CTRLS_MASK, _xklXkb );

  if( _xklLastErrorCode != Success )
  {
    _xklLastErrorMsg = "Could not load controls";
    return False;
  }

  XklDebug( 200, "found %d groups\n", _xklXkb->ctrls->num_groups );

  _xklLastErrorCode = XkbGetNames( _xklDpy, NAMES_MASK, _xklXkb );

  if( _xklLastErrorCode != Success )
  {
    _xklLastErrorMsg = "Could not load names";
    return False;
  }

  gna = _xklXkb->names->groups;
  groupName = groupNames;
  for( i = _xklXkb->ctrls->num_groups; --i >= 0; gna++, groupName++ )
  {
    *groupName = XGetAtomName( _xklDpy,
                               *gna == None ?
                               XInternAtom( _xklDpy, "-", False ) : *gna );
    XklDebug( 200, "group %d has name [%s]\n", i, *groupName );
  }

  _xklLastErrorCode =
    XkbGetIndicatorMap( _xklDpy, XkbAllIndicatorsMask, _xklXkb );

  if( _xklLastErrorCode != Success )
  {
    _xklLastErrorMsg = "Could not load indicator map";
    return False;
  }

  for( i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1 )
  {
    Atom a = _xklXkb->names->indicators[i];
    if( a != None )
      _xklIndicatorNames[i] = XGetAtomName( _xklDpy, a );
    else
      _xklIndicatorNames[i] = "";

    XklDebug( 200, "Indicator[%d] is %s\n", i, _xklIndicatorNames[i] );
  }

  XklDebug( 200, "Real indicators are %X\n",
            _xklXkb->indicators->phys_indicators );

  if( _xklConfigCallback != NULL )
    ( *_xklConfigCallback ) ( _xklConfigCallbackData );

  return True;
}

/**
 * Gets the state from the window property
 */
Bool _XklGetAppState( Window appWin, XklState * state_return )
{
  Atom type_ret;
  int format_ret;
  unsigned long nitems, rest;
  CARD32 *prop = NULL;
  Bool ret = False;

  int grp = -1;
  unsigned inds = -1;

  if( ( XGetWindowProperty
        ( _xklDpy, appWin, _xklAtoms[XKLAVIER_STATE], 0L, 
          XKLAVIER_STATE_PROP_LENGTH, False,
          XA_INTEGER, &type_ret, &format_ret, &nitems, &rest,
          ( unsigned char ** ) &prop ) == Success )
      && ( type_ret == XA_INTEGER ) && ( format_ret == 32 ) )
  {
    grp = prop[0];
    if( grp >= _xklXkb->ctrls->num_groups || grp < 0 )
      grp = 0;

    inds = prop[1];

    if( state_return != NULL )
    {
      state_return->group = grp;
      state_return->indicators = inds;
    }
    if( prop != NULL )
      XFree( prop );

    ret = True;
  }

  if( ret )
    XklDebug( 150,
              "Appwin " WINID_FORMAT
              ", '%s' has the group %d, indicators %X\n", appWin,
              _XklGetDebugWindowTitle( appWin ), grp, inds );
  else
    XklDebug( 150, "Appwin " WINID_FORMAT ", '%s' does not have state\n",
              appWin, _XklGetDebugWindowTitle( appWin ) );

  return ret;
}

void XklLockGroup( int group )
{
  XklDebug( 100, "Posted request for change the group to %d ##\n", group );
  XkbLockGroup( _xklDpy, XkbUseCoreKbd, group );
  XSync( _xklDpy, False );
}

/**
 * Updates current internal state from X state
 */
void _XklGetRealState( XklState * curState_return )
{
  XkbStateRec state;

  curState_return->group = 0;
  if( Success == XkbGetState( _xklDpy, XkbUseCoreKbd, &state ) )
    curState_return->group = state.locked_group;

  if( Success ==
      XkbGetIndicatorState( _xklDpy, XkbUseCoreKbd,
                            &curState_return->indicators ) )
    curState_return->indicators &= _xklXkb->indicators->phys_indicators;
  else
    curState_return->indicators = 0;

}

/*
 * Actually taken from mxkbledpanel, valueChangedProc
 */
Bool _XklSetIndicator( int indicatorNum, Bool set )
{
  XkbIndicatorMapPtr map;

  map = _xklXkb->indicators->maps + indicatorNum;

  /* The 'flags' field tells whether this indicator is automatic
   * (XkbIM_NoExplicit - 0x80), explicit (XkbIM_NoAutomatic - 0x40),
   * or neither (both - 0xC0).
   *
   * If NoAutomatic is set, the server ignores the rest of the 
   * fields in the indicator map (i.e. it disables automatic control 
   * of the LED).   If NoExplicit is set, the server prevents clients 
   * from explicitly changing the value of the LED (using the core 
   * protocol *or* XKB).   If NoAutomatic *and* NoExplicit are set, 
   * the LED cannot be changed (unless you change the map first).   
   * If neither NoAutomatic nor NoExplicit are set, the server will 
   * change the LED according to the indicator map, but clients can 
   * override that (until the next automatic change) using the core 
   * protocol or XKB.
   */
  switch ( map->flags & ( XkbIM_NoExplicit | XkbIM_NoAutomatic ) )
  {
    case XkbIM_NoExplicit | XkbIM_NoAutomatic:
    {
      // Can do nothing. Just ignore the indicator
      return True;
    }

    case XkbIM_NoAutomatic:
    {
      if( _xklXkb->names->indicators[indicatorNum] != None )
        XkbSetNamedIndicator( _xklDpy, XkbUseCoreKbd,
                              _xklXkb->names->indicators[indicatorNum], set,
                              False, NULL );
      else
      {
        XKeyboardControl xkc;
        xkc.led = indicatorNum;
        xkc.led_mode = set ? LedModeOn : LedModeOff;
        XChangeKeyboardControl( _xklDpy, KBLed | KBLedMode, &xkc );
        XSync( _xklDpy, 0 );
      }

      return True;
    }

    case XkbIM_NoExplicit:
      break;
  }

  /* The 'ctrls' field tells what controls tell this indicator to
   * to turn on:  RepeatKeys (0x1), SlowKeys (0x2), BounceKeys (0x4),
   *              StickyKeys (0x8), MouseKeys (0x10), AccessXKeys (0x20),
   *              TimeOut (0x40), Feedback (0x80), ToggleKeys (0x100),
   *              Overlay1 (0x200), Overlay2 (0x400), GroupsWrap (0x800),
   *              InternalMods (0x1000), IgnoreLockMods (0x2000),
   *              PerKeyRepeat (0x3000), or ControlsEnabled (0x4000)
   */
  if( map->ctrls )
  {
    unsigned long which = map->ctrls;

    XkbGetControls( _xklDpy, XkbAllControlsMask, _xklXkb );
    if( set )
      _xklXkb->ctrls->enabled_ctrls |= which;
    else
      _xklXkb->ctrls->enabled_ctrls &= ~which;
    XkbSetControls( _xklDpy, which | XkbControlsEnabledMask, _xklXkb );
  }

  /* The 'which_groups' field tells when this indicator turns on
   *      * for the 'groups' field:  base (0x1), latched (0x2), locked (0x4),
   *           *                          or effective (0x8).
   *                */
  if( map->groups )
  {
    int i;
    unsigned int group = 1;

    /* Turning on a group indicator is kind of tricky.  For
     * now, we will just Latch or Lock the first group we find
     * if that is what this indicator does.  Otherwise, we're
     * just going to punt and get out of here.
     */
    if( set )
    {
      for( i = XkbNumKbdGroups; --i >= 0; )
        if( ( 1 << i ) & map->groups )
        {
          group = i;
          break;
        }
      if( map->which_groups & ( XkbIM_UseLocked | XkbIM_UseEffective ) )
      {
        // Important: Groups should be ignored here - because they are handled separately!
        // XklLockGroup( group );
      } else if( map->which_groups & XkbIM_UseLatched )
        XkbLatchGroup( _xklDpy, XkbUseCoreKbd, group );
      else
      {
        // Can do nothing. Just ignore the indicator
        return True;
      }
    } else
      /* Turning off a group indicator will mean that we just
       * Lock the first group that this indicator doesn't watch.
       */
    {
      for( i = XkbNumKbdGroups; --i >= 0; )
        if( !( ( 1 << i ) & map->groups ) )
        {
          group = i;
          break;
        }
      XklLockGroup( group );
    }
  }

  /* The 'which_mods' field tells when this indicator turns on
   * for the modifiers:  base (0x1), latched (0x2), locked (0x4),
   *                     or effective (0x8).
   *
   * The 'real_mods' field tells whether this turns on when one of 
   * the real X modifiers is set:  Shift (0x1), Lock (0x2), Control (0x4),
   * Mod1 (0x8), Mod2 (0x10), Mod3 (0x20), Mod4 (0x40), or Mod5 (0x80). 
   *
   * The 'virtual_mods' field tells whether this turns on when one of
   * the virtual modifiers is set.
   *
   * The 'mask' field tells what real X modifiers the virtual_modifiers
   * map to?
   */
  if( map->mods.real_mods || map->mods.mask )
  {
    unsigned int affect, mods;

    affect = ( map->mods.real_mods | map->mods.mask );

    mods = set ? affect : 0;

    if( map->which_mods & ( XkbIM_UseLocked | XkbIM_UseEffective ) )
      XkbLockModifiers( _xklDpy, XkbUseCoreKbd, affect, mods );
    else if( map->which_mods & XkbIM_UseLatched )
      XkbLatchModifiers( _xklDpy, XkbUseCoreKbd, affect, mods );
    else
    {
      return True;
    }
  }

  return True;
}

// taken from XFree86 maprules.c
Bool XklGetNamesProp( Atom rulesAtom,
                      char **rulesFileOut, XklConfigRecPtr data )
{
  Atom realPropType;
  int fmt;
  unsigned long nitems, extraBytes;
  char *propData, *out;
  Status rtrn;

  // no such atom!
  if( rulesAtom == None )       /* property cannot exist */
  {
    _xklLastErrorMsg = "Could not find the atom";
    return False;
  }

  rtrn =
    XGetWindowProperty( _xklDpy, _xklRootWindow, rulesAtom, 0L,
                        _XKB_RF_NAMES_PROP_MAXLEN, False, XA_STRING,
                        &realPropType, &fmt, &nitems, &extraBytes,
                        ( unsigned char ** ) &propData );
  // property not found!
  if( rtrn != Success )
  {
    _xklLastErrorMsg = "Could not get the property";
    return False;
  }
  // set rules file to ""
  if( rulesFileOut )
    *rulesFileOut = NULL;

  // has to be array of strings
  if( ( extraBytes > 0 ) || ( realPropType != XA_STRING ) || ( fmt != 8 ) )
  {
    if( propData )
      XFree( propData );
    _xklLastErrorMsg = "Wrong property format";
    return False;
  }
  // rules file
  out = propData;
  if( out && ( *out ) && rulesFileOut )
    *rulesFileOut = strdup( out );
  out += strlen( out ) + 1;

  if( ( out - propData ) < nitems )
  {
    if( *out )
      data->model = strdup( out );
    out += strlen( out ) + 1;
  }

  if( ( out - propData ) < nitems )
  {
    _XklConfigRecSplitLayouts( data, out );
    out += strlen( out ) + 1;
  }

  if( ( out - propData ) < nitems )
  {
    int i;
    char **theLayout, **theVariant;
    _XklConfigRecSplitVariants( data, out );
    /*
       Now have to ensure that number of variants matches the number of layouts
       The 'remainder' is filled with NULLs (not ""s!)
     */
    if( data->numVariants < data->numLayouts )
    {
      data->variants =
        realloc( data->variants, data->numLayouts * sizeof( char * ) );
      memset( data->variants + data->numVariants, 0,
              ( data->numLayouts - data->numVariants ) * sizeof( char * ) );
      data->numVariants = data->numLayouts;
    }
    // take variants from layouts like ru(winkeys)
    theLayout = data->layouts;
    theVariant = data->variants;
    for( i = data->numLayouts; --i >= 0; theLayout++, theVariant++ )
    {
      if( *theLayout != NULL )
      {
        char *varstart = strchr( *theLayout, '(' );
        if( varstart != NULL )
        {
          char *varend = strchr( varstart, ')' );
          if( varend != NULL )
          {
            int varlen = varend - varstart;
            int laylen = varstart - *theLayout;
            // I am not sure - but I assume variants in layout have priority
            char *var = *theVariant = ( *theVariant != NULL ) ?
              realloc( *theVariant, varlen ) : malloc( varlen );
            memcpy( var, varstart + 1, --varlen );
            var[varlen] = '\0';
            realloc( *theLayout, laylen + 1 );
            ( *theLayout )[laylen] = '\0';
          }
        }
      }
    }
    out += strlen( out ) + 1;
  }

  if( ( out - propData ) < nitems )
  {
    _XklConfigRecSplitOptions( data, out );
//    out += strlen( out ) + 1;
  }
  XFree( propData );
  return True;
}

// taken from XFree86 maprules.c
Bool XklSetNamesProp( Atom rulesAtom,
                      char *rulesFile, const XklConfigRecPtr data )
{
  int len, i, rv;
  char *pval;
  char *next;
  char *allLayouts = _XklConfigRecMergeLayouts( data );
  char *allVariants = _XklConfigRecMergeVariants( data );
  char *allOptions = _XklConfigRecMergeOptions( data );

  len = ( rulesFile ? strlen( rulesFile ) : 0 );
  len += ( data->model ? strlen( data->model ) : 0 );
  len += ( allLayouts ? strlen( allLayouts ) : 0 );
  len += ( allVariants ? strlen( allVariants ) : 0 );
  len += ( allOptions ? strlen( allOptions ) : 0 );
  if( len < 1 )
    return True;

  len += 5;                     /* trailing NULs */

  pval = next = ( char * ) malloc( len + 1 );
  if( !pval )
  {
    _xklLastErrorMsg = "Could not allocate buffer";
    return False;
  }
  if( rulesFile )
  {
    strcpy( next, rulesFile );
    next += strlen( rulesFile );
  }
  *next++ = '\0';
  if( data->model )
  {
    strcpy( next, data->model );
    next += strlen( data->model );
  }
  *next++ = '\0';
  if( data->layouts )
  {
    strcpy( next, allLayouts );
    next += strlen( allLayouts );
  }
  *next++ = '\0';
  if( data->variants )
  {
    strcpy( next, allVariants );
    next += strlen( allVariants );
  }
  *next++ = '\0';
  if( data->options )
  {
    strcpy( next, allOptions );
    next += strlen( allOptions );
  }
  *next++ = '\0';
  if( ( next - pval ) != len )
  {
    XklDebug( 150, "Illegal final position: %d/%d\n", ( next - pval ), len );
    if( allOptions != NULL )
      free( allOptions );
    free( pval );
    _xklLastErrorMsg = "Internal property parsing error";
    return False;
  }

  rv = XChangeProperty( _xklDpy, _xklRootWindow, rulesAtom, XA_STRING, 8,
                        PropModeReplace, ( unsigned char * ) pval, len );
  XSync( _xklDpy, False );
#if 0
  for( i = len - 1; --i >= 0; )
    if( pval[i] == '\0' )
      pval[i] = '?';
  XklDebug( 150, "Stored [%s] of length %d to [%s] of %X: %d\n", pval, len,
            propName, _xklRootWindow, rv );
#endif
  if( allOptions != NULL )
    free( allOptions );
  free( pval );
  return True;
}
