#include <errno.h>
#include <glib.h>
#include <jansson.h>
#include <stdbool.h>

#include "util/log.h"

struct wkline;
struct widget;

json_t* load_config_file ();
json_t* wkline_get_config (struct wkline *wkline, const char *key);
json_t* widget_get_config (struct widget *widget, const char *key, bool silent);

#define widget_get_config_string(WIDGET, KEY)  json_string_value(widget_get_config(WIDGET, KEY, false))
#define widget_get_config_integer(WIDGET, KEY) json_integer_value(widget_get_config(WIDGET, KEY, false))
#define widget_get_config_real(WIDGET, KEY)    json_real_value(widget_get_config(WIDGET, KEY, false))
#define widget_get_config_boolean(WIDGET, KEY) json_is_true(widget_get_config(WIDGET, KEY, false))

#define widget_get_config_string_silent(WIDGET, KEY)  json_string_value(widget_get_config(WIDGET, KEY, true))
#define widget_get_config_integer_silent(WIDGET, KEY) json_integer_value(widget_get_config(WIDGET, KEY, true))
#define widget_get_config_real_silent(WIDGET, KEY)    json_real_value(widget_get_config(WIDGET, KEY, true))
#define widget_get_config_boolean_silent(WIDGET, KEY) json_is_true(widget_get_config(WIDGET, KEY, true))
