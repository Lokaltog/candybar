#include "config.h"
#include "widgets.h"
#include "wkline.h"

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
			LOG_ERR("error in config file: %s", err.text);
			exit(EXIT_FAILURE);
		}
		else {
			/* file not found
			   go through all the paths in system_config_dirs */
			for (paths_array = g_get_system_config_dirs(); *paths_array; paths_array++) {
				g_free(config_filename);
				config_filename = g_build_filename(*paths_array, PACKAGE, "config.json", NULL);
				json_config = json_load_file(config_filename, 0, &err);
				if (!json_config) {
					if (err.line != -1) {
						/* syntax error */
						LOG_ERR("error in config file: %s", err.text);
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
wkline_get_config (struct wkline *wkline, const char *config_key) {
	json_t *object;
	object = json_object_get(wkline->config, config_key);
	if (!object) {
		LOG_WARN("wkline config key '%s' not found in config file", config_key);
	}

	return object;
}

json_t*
widget_get_config (struct widget *widget, const char *config_key, bool silent) {
	json_t *object;
	object = json_object_get(widget->config, config_key);
	if (!object && !silent) {
		LOG_WARN("widget config key '%s' in widget '%s' not found in config file", config_key, widget->name);
	}

	return object;
}
