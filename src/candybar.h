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
	CANDYBAR_POSITION_UNKNOWN,
	CANDYBAR_POSITION_TOP,
	CANDYBAR_POSITION_BOTTOM,
} candybar_position_t;

struct candybar {
	WebKitWebView *web_view;
	candybar_position_t position;
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
