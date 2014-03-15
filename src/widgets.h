#ifndef WIDGETS_H
#define WIDGETS_H

#include <gmodule.h>
#include <jansson.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <webkit/webkit.h>

#include "util/config.h"
#include "util/copy_prop.h"
#include "util/gdk_helpers.h"
#include "util/log.h"

struct wkline;
struct widget {
	const char *name;
	json_t *json_config;
	WebKitWebView *web_view;
	char *data;
	struct wkline *wkline;
};

typedef void (*widget_init_func)(void*);

gboolean web_view_update_widget (struct widget *widget);
void handle_interrupt (int signal);
void window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data);

#define widget_init_config_string(WIDGET, KEY, TARGET) \
	{ json_t *CONF = widget_get_config(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_string_value(CONF); } }
#define widget_init_config_integer(WIDGET, KEY, TARGET) \
	{ json_t *CONF = widget_get_config(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_integer_value(CONF); } }
#define widget_init_config_real(WIDGET, KEY, TARGET) \
	{ json_t *CONF = widget_get_config(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_real_value(CONF); } }
#define widget_init_config_boolean(WIDGET, KEY, TARGET) \
	{ json_t *CONF = widget_get_config(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_is_true(CONF); } }

#define widget_send_update(DATA_OBJECT, WIDGET) \
	WIDGET->data = strdup(json_dumps(DATA_OBJECT, 0)); \
	g_idle_add((GSourceFunc)web_view_update_widget, WIDGET); \
	json_decref(DATA_OBJECT);

#endif
