#define _GNU_SOURCE
#include <gmodule.h>
#include <jansson.h>
#include <pthread.h>
#include <string.h>
#include <webkit/webkit.h>

#include "util/log.h"

struct widget {
	const char *name;
	json_t *json_config;
	WebKitWebView *web_view;
	char *data;
};

typedef void (*widget_init_func)(struct widget *widget);

gboolean update_widget (struct widget *widget);
pthread_t spawn_widget (WebKitWebView *web_view, json_t *config, const char *name);
void handle_interrupt (int signal);
void window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data);

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])

#define widget_init_config_string(WIDGET, KEY, TARGET) \
	{ const char *CONF = widget_get_config_string_silent(WIDGET, KEY); \
	  if (CONF) { TARGET = CONF; } }
#define widget_init_config_integer(WIDGET, KEY, TARGET) \
	{ int CONF = widget_get_config_integer_silent(WIDGET, KEY); \
	  if (CONF) { TARGET = CONF; } }
#define widget_init_config_real(WIDGET, KEY, TARGET) \
	{ double CONF = widget_get_config_real_silent(WIDGET, KEY); \
	  if (CONF) { TARGET = CONF; } }
#define widget_init_config_boolean(WIDGET, KEY, TARGET) \
	{ bool CONF = widget_get_config_boolean_silent(WIDGET, KEY); \
	  if (CONF) { TARGET = CONF; } }

#define widget_send_update(DATA_OBJECT, WIDGET) \
	WIDGET->data = strdup(json_dumps(DATA_OBJECT, 0)); \
	g_idle_add((GSourceFunc)update_widget, WIDGET); \
	json_decref(DATA_OBJECT);
