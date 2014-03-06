#include "load_config.h"
#include "src/widgets/widgets.h"
#include "src/wkline.h"

json_t*
load_config_file () {
	const char *user_config_file;
	json_t *json_config;
	json_error_t err;
	user_config_file = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.json", NULL);

	json_config = json_load_file(user_config_file, 0, &err);
	if (!json_config) {
		fprintf(stderr,
		        "***************************\n"
		        "Failed to load config file!\n"
		        "***************************\n\n"
		        "%s\n\n"
		        "If this is your first time using wkline, please copy or link 'config.def.json'\n"
		        "to '%s'.\n\n",
		        err.text, user_config_file);
		exit(EXIT_FAILURE);
	}

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
		wklog("config error: widgets object not found");
	}
	object = json_object_get(object, self->name);

	return json_object_get(object, config_name);
}
