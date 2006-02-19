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
xkl_xmm_config_registry_load(void)
{
	struct stat stat_buf;
	gchar file_name[MAXPATHLEN] = "";
	gchar *rf = xkl_rules_set_get_name("");

	if (rf == NULL || rf[0] == '\0')
		return FALSE;

	g_snprintf(file_name, sizeof file_name, XMODMAP_BASE "/%s.xml",
		   rf);

	if (stat(file_name, &stat_buf) != 0) {
		xkl_last_error_message = "No rules file found";
		return FALSE;
	}

	return xkl_config_registry_load_from_file(file_name);
}

gboolean
xkl_xmm_config_activate(const XklConfigRec * data)
{
	gboolean rv;
	rv = xkl_set_names_prop(xkl_vtable->base_config_atom,
				current_xmm_rules, data);
	if (rv)
		xkl_xmm_group_lock(0);
	return rv;
}
