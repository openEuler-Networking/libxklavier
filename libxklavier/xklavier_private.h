#ifndef __XKLAVIER_PRIVATE_H__
#define __XKLAVIER_PRIVATE_H__

#include <stdio.h>

#include <libxklavier/xklavier_config.h>

typedef Bool ( *XklConfigActivateHandler )( const XklConfigRecPtr data );

typedef void ( *XklConfigInitHandler )( void );

typedef Bool ( *XklConfigMultipleLayoutsSupportedHandler )( void );

typedef Bool ( *XklConfigWriteFileHandler )( const char *fileName,
                                             const XklConfigRecPtr data,
                                             const Bool binary );

typedef int ( *XklEventHandler )( XEvent *xev );

typedef void ( *XklFreeAllInfoHandler )( void );

typedef const char **( *XklGetGroupNamesHandler )( void );

typedef unsigned ( *XklGetNumGroupsHandler )( void );

typedef Bool ( *XklLoadAllInfoHandler )( void );

typedef void ( *XklLockGroupHandler )( int group );

typedef int ( *XklPauseResumeListenHandler )( void );

typedef void ( *XklSetIndicatorsHandler )( const XklState *windowState );

typedef struct
{
  /**
   * Activates the configuration.
   * xkb: create proper the XkbDescRec and send it to the server
   * TODO: xmodmap
   */
  XklConfigActivateHandler xklConfigActivateHandler;
  /**
   * Background-specific initialization.
   * xkb: XkbInitAtoms - init internal xkb atoms table
   * TODO: xmodmap
   */
  XklConfigInitHandler xklConfigInitHandler; /* private */
  /**
   * Can the system combine layouts in one configuration - or not?
   * xkb: checks the simple rule with 2 layouts
   * xmodmap: return true
   */
  XklConfigMultipleLayoutsSupportedHandler xklConfigMultipleLayoutsSupportedHandler;
  /**
   * Write the configuration into the file (binary/textual)
   * xkb: write xkb or xkm file
   * TODO: xmodmap
   */
  XklConfigWriteFileHandler xklConfigWriteFileHandler;
  /**
   * Handles X events.
   * xkb: XkbEvent handling
   * TODO: xmodmap: .... (scariest thing)
   */
  XklEventHandler xklEventHandler;
  /**
   * Flushes the cached server config info.
   * xkb: frees XkbDesc
   * TODO: xmodmap
   */
  XklFreeAllInfoHandler xklFreeAllInfoHandler; /* private */
  /**
   * Get the list of the group names
   * xkb: return cached list of the group names
   * TODO: xmodmap
   */
  XklGetGroupNamesHandler xklGetGroupNamesHandler;
  /**
   * Get the number of loaded groups
   * xkb: return from the cached XkbDesc
   * TODO: xmodmap
   */
  XklGetNumGroupsHandler xklGetNumGroupsHandler;
  /**
   * Loads the configuration info from the server
   * xkb: loads XkbDesc, names, indicators
   * TODO: xmodmap
   */
  XklLoadAllInfoHandler xklLoadAllInfoHandler; /* private */
  /**
   * Switches the keyboard to the group N
   * xkb: simple one-liner to call the XKB function
   * TODO: xmodmap
   */
  XklLockGroupHandler xklLockGroupHandler;
  /**
   * Stop tracking the keyboard-related events
   * xkb: XkbSelectEvents(..., 0)
   * TODO: xmodmap
   */
  XklPauseResumeListenHandler xklPauseListenHandler;
  /**
   * Start tracking the keyboard-related events
   * xkb: XkbSelectEvents + XkbSelectEventDetails + GetRealState
   * TODO: xmodmap
   */
  XklPauseResumeListenHandler xklResumeListenHandler;
  /**
   * Set the indicators state from the XklState
   * xkb: _XklSetIndicator for all indicators
   * TODO: xmodmap
   */
  XklSetIndicatorsHandler xklSetIndicatorsHandler; /* private */
} XklVTable;

extern void _XklEnsureVTableInited( void );

extern void _XklGetRealState( XklState * curState_return );
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

extern const char *_XklGetEventName( int type );

extern Bool _XklIsTransparentAppWindow( Window appWin );

extern void _XklUpdateCurState( int group, unsigned indicators, const char reason[] );

extern int _XklXkbInit( void );

extern int _XklXmmInit( void );

extern Display *_xklDpy;

extern Window _xklRootWindow;

extern XklState _xklCurState;

extern Window _xklCurClient;

extern Status _xklLastErrorCode;

extern const char *_xklLastErrorMsg;

extern XErrorHandler _xklDefaultErrHandler;

extern char *_xklIndicatorNames[];

#define WM_NAME 0
#define WM_STATE 1
#define XKLAVIER_STATE 2
#define XKLAVIER_TRANSPARENT 3

// XKB ones
#define XKB_RF_NAMES_PROP_ATOM 4
#define XKB_RF_NAMES_PROP_ATOM_BACKUP 5
#define TOTAL_ATOMS 6

#define XKLAVIER_STATE_PROP_LENGTH 2

// taken from XFree86 maprules.c
#define _XKB_RF_NAMES_PROP_MAXLEN 1024

extern Atom _xklAtoms[];

extern Bool _xklAllowSecondaryGroupOnce;

extern int _xklDefaultGroup;

extern Bool _xklSkipOneRestore;

extern int _xklSecondaryGroupsMask;

extern int _xklDebugLevel;

extern Window _xklPrevAppWindow;

#define WINID_FORMAT "%lx"

extern XklConfigCallback _xklConfigCallback;

extern void *_xklConfigCallbackData;

extern XklVTable *xklVTable;

#endif
