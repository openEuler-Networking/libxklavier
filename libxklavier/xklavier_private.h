#ifndef __XKLAVIER_PRIVATE_H__
#define __XKLAVIER_PRIVATE_H__

#include <stdio.h>

#include <libxklavier/xklavier_config.h>

typedef Bool ( *XklConfigActivateHandler )( const XklConfigRecPtr data );

typedef void ( *XklConfigInitHandler )( void );

typedef Bool ( *XklConfigLoadRegistryHandler )( void );

typedef Bool ( *XklConfigWriteFileHandler )( const char *fileName,
                                             const XklConfigRecPtr data,
                                             const Bool binary );

typedef int ( *XklEventHandler )( XEvent *xev );

typedef void ( *XklFreeAllInfoHandler )( void );

typedef const char **( *XklGetGroupNamesHandler )( void );

typedef unsigned ( *XklGetMaxNumGroupsHandler )( void );

typedef unsigned ( *XklGetNumGroupsHandler )( void );

typedef void ( *XklGetRealStateHandler)( XklState * curState_return );

typedef Bool ( *XklIfCachedInfoEqualsActualHandler) ( void );

typedef Bool ( *XklLoadAllInfoHandler )( void );

typedef void ( *XklLockGroupHandler )( int group );

typedef int ( *XklPauseResumeListenHandler )( void );

typedef void ( *XklSetIndicatorsHandler )( const XklState *windowState );

typedef struct
{
  /**
   * Backend name
   */
  const char *id;

  /**
   * Functions supported by the backend, combination of XKLF_* constants
   */
  int features;

  /**
   * Activates the configuration.
   * xkb: create proper the XkbDescRec and send it to the server
   * xmodmap: save the property, init layout #1
   */
  XklConfigActivateHandler xklConfigActivateHandler;

  /**
   * Background-specific initialization.
   * xkb: XkbInitAtoms - init internal xkb atoms table
   * xmodmap: void.
   */
  XklConfigInitHandler xklConfigInitHandler; /* private */

  /**
   * Loads the registry tree into DOM (using whatever path(s))
   * The XklConfigFreeRegistry is static - no virtualization necessary.
   * xkb: loads xml from XKB_BASE+"/rules/"+ruleset+".xml"
   * xmodmap: loads xml from XMODMAP_BASE+"/"+ruleset+".xml"
   */
  XklConfigLoadRegistryHandler xklConfigLoadRegistryHandler;

  /**
   * Write the configuration into the file (binary/textual)
   * xkb: write xkb or xkm file
   * xmodmap: if text requested, just dump XklConfigRec to the 
   * file - not really useful. If binary - fail (not supported)
   */
  XklConfigWriteFileHandler xklConfigWriteFileHandler;

  /**
   * Handles X events.
   * xkb: XkbEvent handling
   * xmodmap: keep track on the root window properties. What else can we do?
   */
  XklEventHandler xklEventHandler;

  /**
   * Flushes the cached server config info.
   * xkb: frees XkbDesc
   * xmodmap: frees internal XklConfigRec
   */
  XklFreeAllInfoHandler xklFreeAllInfoHandler; /* private */

  /**
   * Get the list of the group names
   * xkb: return cached list of the group names
   * xmodmap: return the list of layouts from the internal XklConfigRec
   */
  XklGetGroupNamesHandler xklGetGroupNamesHandler;

  /**
   * Get the maximum number of loaded groups
   * xkb: returns 1 or XkbNumKbdGroups
   * xmodmap: return 0
   */
  XklGetMaxNumGroupsHandler xklGetMaxNumGroupsHandler;

  /**
   * Get the number of loaded groups
   * xkb: return from the cached XkbDesc
   * xmodmap: return number of layouts from internal XklConfigRec
   */
  XklGetNumGroupsHandler xklGetNumGroupsHandler;

  /**
   * Gets the current stateCallback
   * xkb: XkbGetState and XkbGetIndicatorState
   * xmodmap: check the root window property (regarding the group)
   */
  XklGetRealStateHandler xklGetRealStateHandler;

  /**
   * Compares the cached info with the actual one, from the server
   * xkb: Compares some parts of XkbDescPtr
   * xmodmap: returns False
   */
  XklIfCachedInfoEqualsActualHandler xklIfCachedInfoEqualsActualHandler;

  /**
   * Loads the configuration info from the server
   * xkb: loads XkbDesc, names, indicators
   * xmodmap: loads internal XklConfigRec from server
   */
  XklLoadAllInfoHandler xklLoadAllInfoHandler; /* private */

  /**
   * Switches the keyboard to the group N
   * xkb: simple one-liner to call the XKB function
   * xmodmap: changes the root window property 
   * (listener invokes xmodmap with appropriate config file).
   */
  XklLockGroupHandler xklLockGroupHandler;

  /**
   * Stop tracking the keyboard-related events
   * xkb: XkbSelectEvents(..., 0)
   * xmodmap: Ungrab the switching shortcut.
   */
  XklPauseResumeListenHandler xklPauseListenHandler;

  /**
   * Start tracking the keyboard-related events
   * xkb: XkbSelectEvents + XkbSelectEventDetails
   * xmodmap: Grab the switching shortcut.
   */
  XklPauseResumeListenHandler xklResumeListenHandler;

  /**
   * Set the indicators state from the XklState
   * xkb: _XklSetIndicator for all indicators
   * xmodmap: NULL. Not supported
   */
  XklSetIndicatorsHandler xklSetIndicatorsHandler; /* private */
  
  /* all data is private - no direct access */
  /**
   * The base configuration atom.
   * xkb: _XKB_RF_NAMES_PROP_ATOM
   * xmodmap:  "_XMM_NAMES"
   */
  Atom baseConfigAtom;
  
  /**
   * The configuration backup atom
   * xkb: "_XKB_RULES_NAMES_BACKUP"
   * xmodmap: "_XMM_NAMES_BACKUP"
   */
  Atom backupConfigAtom;
  
  /**
   * Fallback for missing model
   */
  const char* defaultModel;

  /**
   * Fallback for missing layout
   */
  const char* defaultLayout;
  
} XklVTable;

extern void _XklEnsureVTableInited( void );

extern void _XklAddAppWindow( Window win, Window parent, Bool force,
                              XklState * initState );
extern Bool _XklGetAppWindowBottomToTop( Window win, Window * appWin_return );
extern Bool _XklGetAppWindow( Window win, Window * appWin_return );

extern void _XklFocusInEvHandler( XFocusChangeEvent * fev );
extern void _XklFocusOutEvHandler( XFocusChangeEvent * fev );
extern void _XklPropertyEvHandler( XPropertyEvent * rev );
extern void _XklCreateEvHandler( XCreateWindowEvent * cev );

extern void _XklErrHandler( Display * dpy, XErrorEvent * evt );

extern Window _XklGetRegisteredParent( Window win );
extern Bool _XklLoadAllInfo( void );
extern void _XklFreeAllInfo( void );
extern void _XklResetAllInfo( const char reason[] );
extern Bool _XklLoadWindowTree( void );
extern Bool _XklLoadSubtree( Window window, int level, XklState * initState );

extern Bool _XklHasWmState( Window win );

extern Bool _XklGetAppState( Window appWin, XklState * state_return );
extern void _XklDelAppState( Window appWin );
extern void _XklSaveAppState( Window appWin, XklState * state );

extern void _XklSelectInputMerging( Window win, long mask );

extern char *_XklGetDebugWindowTitle( Window win );

extern Status _XklStatusQueryTree( Display * display,
                                   Window w,
                                   Window * root_return,
                                   Window * parent_return,
                                   Window ** children_return,
                                   signed int *nchildren_return );

extern Bool _XklSetIndicator( int indicatorNum, Bool set );

extern void _XklTryCallStateCallback( XklStateChange changeType,
                                      XklState * oldState );

extern void _XklI18NInit(  );

extern char *_XklLocaleFromUtf8( const char *utf8string );

extern int _XklGetLanguagePriority( const char *language );

extern char* _XklGetRulesSetName( const char defaultRuleset[] );

extern Bool _XklConfigGetFullFromServer( char **rulesFileOut, 
                                         XklConfigRecPtr data );

extern char *_XklConfigRecMergeByComma( const char **array,
                                        const int arrayLength );

extern char *_XklConfigRecMergeLayouts( const XklConfigRecPtr data );

extern char *_XklConfigRecMergeVariants( const XklConfigRecPtr data );

extern char *_XklConfigRecMergeOptions( const XklConfigRecPtr data );

extern void _XklConfigRecSplitByComma( char ***array,
                                       int *arraySize, const char *merged );

extern void _XklConfigRecSplitLayouts( XklConfigRecPtr data,
                                       const char *merged );

extern void _XklConfigRecSplitVariants( XklConfigRecPtr data,
                                        const char *merged );

extern void _XklConfigRecSplitOptions( XklConfigRecPtr data,
                                       const char *merged );

extern void XklConfigDump( FILE* file,
                           XklConfigRecPtr data );
                           
extern const char *_XklGetEventName( int type );

extern Bool _XklIsTransparentAppWindow( Window appWin );

extern void _XklUpdateCurState( int group, unsigned indicators, const char reason[] );

extern void _XklStateModificationHandler( XklStateChange changeType, 
                                          int grp, 
                                          unsigned inds,
                                          Bool setInds );

extern int _XklXkbInit( void );

extern int _XklXmmInit( void );

extern Bool _XklIsOneSwitchToSecondaryGroupAllowed( void );

extern void _XklOneSwitchToSecondaryGroupPerformed( void );

extern Display *_xklDpy;

extern Window _xklRootWindow;

extern XklState _xklCurState;

extern Window _xklCurClient;

extern Status _xklLastErrorCode;

extern const char *_xklLastErrorMsg;

extern XErrorHandler _xklDefaultErrHandler;

extern char *_xklIndicatorNames[];

enum { WM_NAME,
  WM_STATE,
  XKLAVIER_STATE,
  XKLAVIER_TRANSPARENT,
  XKLAVIER_ALLOW_SECONDARY,
  TOTAL_ATOMS };

#define XKLAVIER_STATE_PROP_LENGTH 2

/* taken from XFree86 maprules.c */
#define _XKB_RF_NAMES_PROP_MAXLEN 1024

extern Atom _xklAtoms[];

extern Bool _xklAllowSecondaryGroupOnce;

extern int _xklDefaultGroup;

extern Bool _xklSkipOneRestore;

extern int _xklSecondaryGroupsMask;

extern int _xklDebugLevel;

extern int _xklListenerType;

extern Window _xklPrevAppWindow;

#define WINID_FORMAT "%lx"

extern XklConfigCallback _xklConfigCallback;

extern void *_xklConfigCallbackData;

extern XklVTable *xklVTable;

#ifdef __STRICT_ANSI__
/* these are functions which are NOT in ANSI C. 
   Probably we should provide the implementation */
extern int snprintf( char *s, size_t maxlen,
                     const char *format, ... );
                     
extern char *strdup( const char *s );
#endif

#endif
