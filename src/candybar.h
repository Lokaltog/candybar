#ifndef CANDYBAR_H
#define CANDYBAR_H

#include <errno.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <webkit/webkit.h>

#include "util/config.h"
#include "util/copy_prop.h"
#include "util/gdk_helpers.h"
#include "util/log.h"

typedef enum {
	BAR_POSITION_UNKNOWN,
	BAR_POSITION_TOP,
	BAR_POSITION_BOTTOM,
} bar_position_t;

struct bar {
	WebKitWebView *web_view;
	bar_position_t position;
	int pos_x;
	int pos_y;
	int width;
	int height;
	int monitor;
	json_t *config;
	const char *theme_uri;
	json_t *theme_config;
	int efd;
};

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])

#endif
