#include <glib.h>
#include <jansson.h>

#include "util/log.h"

struct wkline;
struct widget;

json_t* load_config_file ();
void assign_json_value (json_t *object, void *value);
int wkline_get_config (struct wkline *self, const char *config_key, void *value);
int widget_get_config (struct widget *self, const char *config_key, void *value);

#define CONFIG_VALUE_SIZE 256

enum config_error {
	CONFIG_ERROR_OK,
	CONFIG_ERROR_NO_WIDGETS,
	CONFIG_ERROR_INVALID_WIDGET,
	CONFIG_ERROR_INVALID_KEY,
};
