#ifndef __XKLAVIER_PRIVATE_XKB_H__
#define __XKLAVIER_PRIVATE_XKB_H__

#ifdef XKB_HEADERS_PRESENT

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#define ForPhysIndicators( i, bit ) \
    for ( i=0, bit=1; i<XkbNumIndicators; i++, bit<<=1 ) \
          if ( _xklXkb->indicators->phys_indicators & bit )

extern int _xklXkbEventType, _xklXkbError;

extern XkbRF_VarDefsRec _xklVarDefs;

extern XkbDescPtr _xklXkb;

extern void XklDumpXkbDesc( const char *filename, XkbDescPtr kbd );

/* Start VTable methods */

extern Bool _XklXkbConfigActivate( const XklConfigRecPtr data );

extern void _XklXkbConfigInit( void );

extern Bool _XklXkbConfigLoadRegistry( void );

extern Bool _XklXkbConfigMultipleLayoutsSupported( void );

extern Bool _XklXkbConfigWriteFile( const char *fileName,
                                    const XklConfigRecPtr data,
                                    const Bool binary );

extern int _XklXkbEventHandler( XEvent * kev );

extern void _XklXkbFreeAllInfo( void );

extern const char **_XklXkbGetGroupNames( void );

extern unsigned _XklXkbGetNumGroups( void );

extern Bool _XklXkbLoadAllInfo( void );

extern void _XklXkbLockGroup( int group );

extern int _XklXkbPauseListen( void );

extern int _XklXkbResumeListen( void );

extern void _XklXkbSetIndicators( const XklState *windowState );

/* End of VTable methods */

#endif

extern Bool _xklXkbExtPresent;

#endif
