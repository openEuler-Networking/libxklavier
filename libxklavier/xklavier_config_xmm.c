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
xkl_xmm_init_config_registry(XklConfigRegistry * config)
{
}

gboolean
xkl_xmm_load_config_registry(XklConfigRegistry * config)
{
	struct stat stat_buf;
	gchar file_name[MAXPATHLEN] = "";
	XklEngine *engine = xkl_config_registry_get_engine(config);
	gchar *rf = xkl_engine_get_ruleset_name(engine, "");

	if (rf == NULL || rf[0] == '\0')
		return FALSE;

	g_snprintf(file_name, sizeof file_name, XMODMAP_BASE "/%s.xml",
		   rf);

	if (stat(file_name, &stat_buf) != 0) {
		xkl_last_error_message = "No rules file found";
		return FALSE;
	}

	return xkl_config_registry_load_from_file(config, file_name);
}

gboolean
xkl_xmm_activate_config_rec(XklEngine * engine, const XklConfigRec * data)
{
	gboolean rv;
	rv = xkl_config_rec_set_to_root_window_property(data,
							xkl_engine_priv
							(engine,
							 base_config_atom),
							xkl_engine_backend
							(engine, XklXmm,
							 current_rules),
							engine);
	if (rv)
		xkl_xmm_lock_group(engine, 0);
	return rv;
}
