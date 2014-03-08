#define _GNU_SOURCE
#include <gmodule.h>
#include <jansson.h>
#include <pthread.h>
#include <string.h>
#include <webkit/webkit.h>

#include "util/log.h"

struct widget {
	const char *name;
	json_t *config;
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
