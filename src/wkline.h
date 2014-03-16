#ifndef WKLINE_H
#define WKLINE_H

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <string.h>
#include <unistd.h>
#include <webkit/webkit.h>

#include "util/config.h"
#include "util/copy_prop.h"
#include "util/gdk_helpers.h"
#include "util/log.h"

typedef enum {
	WKLINE_POSITION_UNKNOWN,
	WKLINE_POSITION_TOP,
	WKLINE_POSITION_BOTTOM,
} wkline_position_t;

struct wkline {
	WebKitWebView *web_view;
	wkline_position_t position;
	int width;
	int height;
	int screen;
	json_t *config;
	const char *theme_uri;
	json_t *theme_config;
	void *wk_context;
};

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])

#endif
