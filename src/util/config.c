#include "config.h"

json_t*
get_config_json (char *config_override_filename) {
	const gchar*const *paths_array;
	gchar *config_filename;
	gchar *override_path;
	json_t *json_config;
	json_error_t err;

	override_path = config_override_filename != NULL
	                ? config_override_filename : getenv("CANDYBAR_CONFIG_PATH");
	if (override_path != NULL) {
		config_filename = g_build_filename(override_path, NULL);

		if (access(config_filename, R_OK) == -1) {
			LOG_ERR("could not open config file '%s' for reading: %s", config_filename, strerror(errno));

			return NULL;
		}
	}
	else {
		config_filename = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.json", NULL);
	}

	LOG_INFO("using config file '%s'", config_filename);

	json_config = json_load_file(config_filename, 0, &err);
	if (!json_config) {
		if (err.line != -1) {
			/* syntax error */
			LOG_ERR("error in config file: %s", err.text);

			return NULL;
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

						return NULL;
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
get_config_option (json_t *config_object, const char *option, bool silent) {
	json_t *value = json_object_get(config_object, option);
	if (!value && !silent) {
		LOG_WARN("option '%s' not found in config object", option);
	}

	return value;
}
