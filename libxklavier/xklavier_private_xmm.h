#ifndef __XKLAVIER_PRIVATE_XMM_H__
#define __XKLAVIER_PRIVATE_XMM_H__

/* Start VTable methods */

extern Bool _XklXmmConfigActivate( const XklConfigRecPtr data );

extern void _XklXmmConfigInit( void );

extern Bool _XklXmmConfigLoadRegistry( void );

extern Bool _XklXmmConfigMultipleLayoutsSupported( void );

extern Bool _XklXmmConfigWriteFile( const char *fileName,
                                    const XklConfigRecPtr data,
                                    const Bool binary );

extern int _XklXmmEventHandler( XEvent * kev );

extern void _XklXmmFreeAllInfo( void );

extern const char **_XklXmmGetGroupNames( void );

extern unsigned _XklXmmGetNumGroups( void );

extern Bool _XklXmmLoadAllInfo( void );

extern void _XklXmmLockGroup( int group );

extern int _XklXmmPauseListen( void );

extern int _XklXmmResumeListen( void );

extern void _XklXmmSetIndicators( const XklState *windowState );

/* End of VTable methods */

#endif
