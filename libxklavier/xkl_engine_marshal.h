
#ifndef __g_cclosure_user_marshal_MARSHAL_H__
#define __g_cclosure_user_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:VOID (marshal.list:1) */
#define g_cclosure_user_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* INT:LONG,LONG (marshal.list:2) */
extern void g_cclosure_user_marshal_INT__LONG_LONG (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

/* VOID:FLAGS,INT,BOOLEAN (marshal.list:3) */
extern void g_cclosure_user_marshal_VOID__FLAGS_INT_BOOLEAN (GClosure     *closure,
                                                             GValue       *return_value,
                                                             guint         n_param_values,
                                                             const GValue *param_values,
                                                             gpointer      invocation_hint,
                                                             gpointer      marshal_data);

G_END_DECLS

#endif /* __g_cclosure_user_marshal_MARSHAL_H__ */

