#ifndef __XKLAVIER_PRIVATE_XMM_H__
#define __XKLAVIER_PRIVATE_XMM_H__

typedef struct _XmmShortcut 
{
  int keysym;
  int modifiers;
} XmmShortcut, *XmmShortcutPtr;

#define MAX_SHORTCUTS_PER_OPTION 4
typedef struct _XmmSwitchOption
{
  const char* optionName;
  int numShortcuts;
  XmmShortcut shortcuts[MAX_SHORTCUTS_PER_OPTION];
  int shortcutSteps[MAX_SHORTCUTS_PER_OPTION];
} XmmSwitchOption, *XmmSwitchOptionPtr;

extern char* currentXmmRules;

extern XklConfigRec currentXmmConfig;

extern Atom xmmStateAtom;

/* in the ideal world this should be a hashmap */
extern XmmSwitchOption allSwitchOptions[];

extern void _XklXmmGrabIgnoringIndicators( int keycode, int modifiers );

extern void _XklXmmUngrabIgnoringIndicators( int keycode, int modifiers );

extern void _XklXmmGrabShortcuts( void );

extern void _XklXmmUngrabShortcuts( void );

extern const char* _XklXmmGetCurrentShortcutOptionName( void );

extern const XmmSwitchOptionPtr _XklXmmGetCurrentShortcut( void );

extern void _XklXmmActualizeGroup( int group );

extern const XmmSwitchOptionPtr _XklXmmFindSwitchOption( unsigned keycode, 
                                                         unsigned state,
                                                         int * currentShortcut_rv );

/* Start VTable methods */

extern Bool _XklXmmConfigActivate( const XklConfigRecPtr data );

extern void _XklXmmConfigInit( void );

extern Bool _XklXmmConfigLoadRegistry( void );

extern int _XklXmmEventHandler( XEvent * kev );

extern void _XklXmmFreeAllInfo( void );

extern const char **_XklXmmGetGroupNames( void );

extern unsigned _XklXmmGetNumGroups( void );

extern void _XklXmmGetRealState( XklState * curState_return );

extern Bool _XklXmmLoadAllInfo( void );

extern void _XklXmmLockGroup( int group );

extern int _XklXmmPauseListen( void );

extern int _XklXmmResumeListen( void );

/* End of VTable methods */

#endif
