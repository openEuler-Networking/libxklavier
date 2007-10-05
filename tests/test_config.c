/*
 * Copyright (C) 2002-2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <libxklavier/xklavier.h>

#ifdef HAVE_SETLOCALE
# include <locale.h>
#endif

extern void xkl_config_rec_dump(FILE * file, XklConfigRec * data);

enum { ACTION_NONE, ACTION_LIST, ACTION_GET, ACTION_SET,
	ACTION_WRITE
};

static void
print_usage(void)
{
	printf
	    ("Usage: test_config (-g)|(-s -m <model> -l <layouts> -o <options>)|(-h)|(-ws)|(-wb)(-d <debugLevel>)\n");
	printf("Options:\n");
	printf("         -al - list all available layouts and variants\n");
	printf("         -am - list all available models\n");
	printf
	    ("         -ao - list all available options groups and options\n");
	printf
	    ("         -g - Dump the current config, load original system settings and revert back\n");
	printf
	    ("         -s - Set the configuration given my -m -l -o options. Similar to setxkbmap\n");
	printf("         -ws - Write the binary XKB config file (" PACKAGE
	       ".xkm)\n");
	printf("         -wb - Write the source XKB config file (" PACKAGE
	       ".xkb)\n");
	printf("         -d - Set the debug level (by default, 0)\n");
	printf("         -h - Show this help\n");
}

static void
print_option(XklConfigRegistry * config, const XklConfigItem * item,
	     gpointer data)
{
	printf("  [%s][%s][%s]\n", item->name,
	       item->description, item->short_description);
}

static void
print_option_group(XklConfigRegistry * config, const XklConfigItem * item,
		   gpointer data)
{
	printf("[%s][%s][%s] %s multiple selection\n", item->name,
	       item->description, item->short_description,
	       GPOINTER_TO_INT(g_object_get_data
			       (G_OBJECT(item),
				XCI_PROP_ALLOW_MULTIPLE_SELECTION)) ?
	       "Allows" : "Does not allow");
	xkl_config_registry_foreach_option(config, item->name,
					   print_option, data);
}

static void
print_model(XklConfigRegistry * config, const XklConfigItem * item,
	    gpointer data)
{
	gchar *vendor =
	    (gchar *) g_object_get_data(G_OBJECT(item), XCI_PROP_VENDOR);
	printf("[%s][%s][%s] by %s\n", item->name, item->description,
	       item->short_description,
	       vendor == NULL ? "unknown" : vendor);
}

static void
print_variant(XklConfigRegistry * config, const XklConfigItem * item,
	      gpointer data)
{
	printf("  [%s][%s][%s]\n", item->name,
	       item->description, item->short_description);
}
static void
print_layout(XklConfigRegistry * config, const XklConfigItem * item,
	     gpointer data)
{
	printf("[%s][%s][%s]\n", item->name, item->description,
	       item->short_description);
	xkl_config_registry_foreach_layout_variant(config, item->name,
						   print_variant, data);
}

int
main(int argc, char *const argv[])
{
	int c;
	gchar which_list = 0;
	int action = ACTION_NONE;
	const gchar *model = NULL;
	const gchar *layouts = NULL;
	const gchar *options = NULL;
	int debug_level = -1;
	int binary = 0;
	Display *dpy;
	XklEngine *engine;

	g_type_init_with_debug_flags(G_TYPE_DEBUG_OBJECTS |
				     G_TYPE_DEBUG_SIGNALS);

	while (1) {
		c = getopt(argc, argv, "ha:sgm:l:o:d:w:c:");
		if (c == -1)
			break;
		switch (c) {
		case 'a':
			which_list = optarg[0];
			printf("List the registry\n");
			action = ACTION_LIST;
			break;
		case 's':
			printf("Set the config\n");
			action = ACTION_SET;
			break;
		case 'g':
			printf("Get the config\n");
			action = ACTION_GET;
			break;
		case 'm':
			printf("Model: [%s]\n", model = optarg);
			break;
		case 'l':
			printf("Layouts: [%s]\n", layouts = optarg);
			break;
		case 'o':
			printf("Options: [%s]\n", options = optarg);
			break;
		case 'h':
			print_usage();
			exit(0);
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 'w':
			action = ACTION_WRITE;
			binary = ('b' == optarg[0]);
		default:
			fprintf(stderr,
				"?? getopt returned character code 0%o ??\n",
				c);
			print_usage();
		}
	}

	if (action == ACTION_NONE) {
		print_usage();
		exit(0);
	}
#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "");
#endif

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Could not open display\n");
		exit(1);
	}
	if (debug_level != -1)
		xkl_set_debug_level(debug_level);
	engine = xkl_engine_get_instance(dpy);
	if (engine != NULL) {
		XklConfigRec *current_config, *r2;
		XklConfigRegistry *config;

		xkl_debug(0, "Xklavier initialized\n");
		config = xkl_config_registry_get_instance(engine);
		xkl_config_registry_load(config);

		xkl_debug(0, "Xklavier registry loaded\n");
		xkl_debug(0, "Backend: [%s]\n",
			  xkl_engine_get_backend_name(engine));
		xkl_debug(0, "Supported features: 0x0%X\n",
			  xkl_engine_get_features(engine));
		xkl_debug(0, "Max number of groups: %d\n",
			  xkl_engine_get_max_num_groups(engine));

		current_config = xkl_config_rec_new();
		xkl_config_rec_get_from_server(current_config, engine);

		switch (action) {
		case ACTION_LIST:
			switch (which_list) {
			case 'l':
				xkl_config_registry_foreach_layout(config,
								   print_layout,
								   NULL);
				break;
			case 'm':
				xkl_config_registry_foreach_model(config,
								  print_model,
								  NULL);
				break;
			case 'o':
				xkl_config_registry_foreach_option_group
				    (config, print_option_group, NULL);
				break;
			default:
				printf("Unknown list: %c\n", which_list);
				print_usage();
				break;
			}
			break;
		case ACTION_GET:
			xkl_debug(0, "Got config from the server\n");
			xkl_config_rec_dump(stdout, current_config);

			r2 = xkl_config_rec_new();

			if (xkl_config_rec_get_from_backup(r2, engine)) {
				xkl_debug(0,
					  "Got config from the backup\n");
				xkl_config_rec_dump(stdout, r2);
			}

			if (xkl_config_rec_activate(r2, engine)) {
				xkl_debug(0,
					  "The backup configuration restored\n");
				if (xkl_config_rec_activate
				    (current_config, engine)) {
					xkl_debug(0,
						  "Reverting the configuration change\n");
				} else {
					xkl_debug(0,
						  "The configuration could not be reverted: %s\n",
						  xkl_get_last_error());
				}
			} else {
				xkl_debug(0,
					  "The backup configuration could not be restored: %s\n",
					  xkl_get_last_error());
			}

			g_object_unref(G_OBJECT(r2));
			break;
		case ACTION_SET:
			if (model != NULL) {
				if (current_config->model != NULL)
					g_free(current_config->model);
				current_config->model = g_strdup(model);
			}

			if (layouts != NULL) {
				if (current_config->layouts != NULL)
					g_strfreev(current_config->
						   layouts);
				if (current_config->variants != NULL)
					g_strfreev(current_config->
						   variants);

				current_config->layouts =
				    g_new0(char *, 2);
				current_config->layouts[0] =
				    g_strdup(layouts);
				current_config->variants =
				    g_new0(char *, 2);
				current_config->variants[0] = g_strdup("");
			}

			if (options != NULL) {
				if (current_config->options != NULL)
					g_strfreev(current_config->
						   options);

				current_config->options =
				    g_new0(char *, 2);
				current_config->options[0] =
				    g_strdup(options);
			}

			xkl_debug(0, "New config:\n");
			xkl_config_rec_dump(stdout, current_config);
			if (xkl_config_rec_activate
			    (current_config, engine))
				xkl_debug(0, "Set the config\n");
			else
				xkl_debug(0,
					  "Could not set the config: %s\n",
					  xkl_get_last_error());
			break;
		case ACTION_WRITE:
			xkl_config_rec_write_to_file(engine,
						     binary ? (PACKAGE
							       ".xkm")
						     : (PACKAGE ".xkb"),
						     current_config,
						     binary);
			xkl_debug(0, "The file " PACKAGE "%s is written\n",
				  binary ? ".xkm" : ".xkb");
			break;
		}

		g_object_unref(G_OBJECT(current_config));

		g_object_unref(G_OBJECT(config));
		xkl_debug(0, "Xklavier registry freed\n");
		xkl_debug(0, "Xklavier terminating\n");
		g_object_unref(G_OBJECT(engine));
	} else {
		fprintf(stderr, "Could not init _xklavier\n");
		exit(2);
	}
	printf("closing display: %p\n", dpy);
	XCloseDisplay(dpy);
	return 0;
}
