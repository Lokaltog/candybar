#include <glib.h>
#include <jansson.h>

#include "config.h"

struct wkline_t;
struct wkline_widget_t;

json_t * load_config_file ();
json_t * wkline_get_config (struct wkline_t *self, const char *config_name);
json_t * wkline_widget_get_config (struct wkline_widget_t *self, const char *config_name);
