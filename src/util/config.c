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

void
assign_json_value (json_t *object, void *value) {
	switch (json_typeof(object)) {
	case JSON_OBJECT:
	case JSON_ARRAY:
		value = object;
		break;
	case JSON_STRING:
		strncpy(value, json_string_value(object), CONFIG_VALUE_SIZE);
		break;
	case JSON_INTEGER:
		*(int*)value = json_integer_value(object);
		break;
	case JSON_REAL:
		*(double*)value = json_integer_value(object);
		break;
	case JSON_TRUE:
		*(bool*)value = true;
		break;
	case JSON_FALSE:
		*(bool*)value = false;
		break;
	case JSON_NULL:
		value = NULL;
		break;
	}
}

int
wkline_get_config (struct wkline *self, const char *config_key, void *value) {
	json_t *object;

	object = json_object_get(self->config, config_key);
	if (!object) {
		return CONFIG_ERROR_INVALID_KEY;
	}

	assign_json_value(object, value);

	return CONFIG_ERROR_OK;
}

int
widget_get_config (struct widget *self, const char *config_key, void *value) {
	json_t *object;

	object = json_object_get(self->config, "widgets_config");
	if (!object) {
		return CONFIG_ERROR_NO_WIDGETS;
	}
	object = json_object_get(object, self->name);
	if (!object) {
		return CONFIG_ERROR_INVALID_WIDGET;
	}
	object = json_object_get(object, config_key);
	if (!object) {
		return CONFIG_ERROR_INVALID_KEY;
	}

	assign_json_value(object, value);

	return CONFIG_ERROR_OK;
}
