#include <errno.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <fcntl.h>

#include <libxml/xpath.h>

#include "config.h"

#include "xklavier_private.h"
#include "xklavier_private_xmm.h"

#define XK_XKB_KEYS
#include <X11/keysymdef.h>

void
xkl_xmm_config_init(void)
{
}

gboolean
xkl_xmm_load_config_registry(XklConfig * config)
{
	struct stat stat_buf;
	gchar file_name[MAXPATHLEN] = "";
	XklEngine *engine = xkl_config_get_engine(config);
	gchar *rf = xkl_engine_get_ruleset_name(engine, "");

	if (rf == NULL || rf[0] == '\0')
		return FALSE;

	g_snprintf(file_name, sizeof file_name, XMODMAP_BASE "/%s.xml",
		   rf);

	if (stat(file_name, &stat_buf) != 0) {
		engine->priv->last_error_message = "No rules file found";
		return FALSE;
	}

	return xkl_config_load_registry_from_file(config, file_name);
}

gboolean
xkl_xmm_activate_config(XklConfig * config, const XklConfigRec * data)
{
	gboolean rv;
	XklEngine *engine = xkl_config_get_engine(config);
	rv = xkl_set_names_prop(engine->priv->base_config_atom,
				current_xmm_rules, data);
	if (rv)
		xkl_xmm_lock_group(engine, 0);
	return rv;
}
