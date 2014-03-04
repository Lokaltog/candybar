#include "load_config.h"
#include "src/widgets/widgets.h"
#include "src/wkline.h"

json_t *
load_config_file () {
	const char *user_config_file;
	json_t *json_config;
	json_error_t err;
	user_config_file = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.json", NULL);

	json_config = json_load_file(user_config_file, 0, &err);
	if (!json_config){
		printf("Error loading config file: %s\n", err.text);
		exit(EXIT_FAILURE);
	}
	return json_config;
}

json_t *
wkline_get_config(struct wkline_t *self, const char *config_name){
	return json_object_get(self->config, config_name);
}

json_t *
wkline_widget_get_config(struct wkline_widget_t *self, const char *config_name){
	json_t *object;
	object = json_object_get(self->config, "widgets");
	if (!object){
		printf("Error loading config file: widgets not found\n");
	}
	object = json_object_get(object, self->name);
	return json_object_get(object, config_name);
}