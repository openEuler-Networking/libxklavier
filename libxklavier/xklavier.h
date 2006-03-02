/**
 * @file xklavier.h
 */

#ifndef __XKLAVIER_H__
#define __XKLAVIER_H__

#include <stdarg.h>

#include <X11/Xlib.h>

#include <glib-object.h>

#include <libxklavier/xkl_engine.h>
#include <libxklavier/xkl_config_rec.h>
#include <libxklavier/xkl_config_item.h>
#include <libxklavier/xkl_config_registry.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup debugerr Debugging, error processing 
 * @{
 */

/**
 * @return the text message (statically allocated) of the last error
 */
	extern const gchar *xkl_get_last_error(void);

/**
 * Output (optionally) some debug info
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @see xkl_debug
 */
	extern void _xkl_debug(const gchar file[], const gchar function[],
			       gint level, const gchar format[], ...);

/**
 * Custom log output method for _xkl_debug. This appender is NOT called if the
 * level of the message is greater than currently set debug level.
 *
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @param args is the list of parameters
 * @see _xkl_debug
 * @see xkl_set_debug_level
 */
	typedef void (*XklLogAppender) (const gchar file[],
					const gchar function[],
					gint level,
					const gchar format[],
					va_list args);

/**
 * Default log output method. Sends everything to stdout.
 *
 * @param file is the name of the source file. 
 * Preprocessor symbol__FILE__ should be used here
 * @param function is a name of the function
 * Preprocessor symbol__func__ should be used here
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @param args is the list of parameters
 */
	extern void xkl_default_log_appender(const gchar file[],
					     const gchar function[],
					     gint level,
					     const gchar format[],
					     va_list args);

/**
 * Installs the custom log appender.function
 * @param fun is the new log appender
 */
	extern void xkl_set_log_appender(XklLogAppender fun);

/**
 * Sets maximum debug level. 
 * Message of the level more than the one set here - will be ignored
 * @param level is a new debug level
 */
	extern void xkl_set_debug_level(gint level);

/* Just to make doxygen happy - two block with/without @param format */
#if defined(G_HAVE_GNUC_VARARGS)
/**
 * Output (optionally) some debug info
 * @param level is a level of the message
 * @param format is a format (like in printf)
 * @see _xkl_Debug
 */
#else
/**
 * Output (optionally) some debug info
 * @param level is a level of the message
 * @see _xkl_Debug
 */
#endif
#ifdef G_HAVE_ISO_VARARGS
#define xkl_debug( level, ... ) \
  _xkl_debug( __FILE__, __func__, level, __VA_ARGS__ )
#elif defined(G_HAVE_GNUC_VARARGS)
#define xkl_debug( level, format, args... ) \
   _xkl_debug( __FILE__, __func__, level, format, ## args )
#else
#define xkl_debug( level, ... ) \
  _xkl_debug( __FILE__, __func__, level, __VA_ARGS__ )
#endif

/** @} */

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
