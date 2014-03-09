#include <glib.h>
#include <jansson.h>

#include "util/log.h"

struct wkline;
struct widget;

json_t* load_config_file ();
json_t* wkline_get_config (struct wkline *self, const char *config_name);
json_t* wkline_widget_get_config (struct widget *self, const char *config_name);
json_t* wkline_widget_get_config_silent (struct widget *self, const char *config_name);
