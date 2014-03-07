#include "config.h"
#include "src/widgets/widgets.h"
#include "src/wkline.h"

json_t*
load_config_file () {
	const gchar*const *paths_array;
	gchar *config_filename;
	json_t *json_config;
	json_error_t err;

	config_filename = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.json", NULL);
	json_config = json_load_file(config_filename, 0, &err);
	if (!json_config) {
		if (err.line != -1) {
			/* syntax error */
			wklog("Error in config file: %s", err.text);
			exit(EXIT_FAILURE);
		}
		else {
			/* file not found
			   go through all the paths in system_config_dirs */
			for (paths_array = g_get_system_config_dirs(); *paths_array; *paths_array++) {
				g_free(config_filename);
				config_filename = g_build_filename(*paths_array, PACKAGE, "config.json", NULL);
				json_config = json_load_file(config_filename, 0, &err);
				if (!json_config) {
					if (err.line != -1) {
						/* syntax error */
						fprintf(stderr, "Error in config file: %s", err.text);
					}
				}
				else {
					/* this file is good */
					break;
				}
			}
		}
	}
	g_free(config_filename);

	return json_config;
}

json_t*
wkline_get_config (struct wkline *self, const char *config_name) {
	return json_object_get(self->config, config_name);
}

json_t*
wkline_widget_get_config (struct widget *self, const char *config_name) {
	json_t *object;
	object = json_object_get(self->config, "widgets");
	if (!object) {
		wklog("Warning: widgets block not found in config file");
	}
	object = json_object_get(object, self->name);
	if (!object) {
		wklog("Warning: widget \"%s\" not found in config file", self->name);
	}
	object = json_object_get(object, config_name);
	if (!object) {
		wklog("Warning: configuration \"%s\" in widget \"%s\" not found in config file", config_name, self->name);
	}

	return object;
}
