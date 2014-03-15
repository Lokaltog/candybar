#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <string.h>
#include <unistd.h>
#include <webkit/webkit.h>

struct wkline_dimensions {
	int w;
	int h;
};

#include "util/config.h"
#include "util/copy_prop.h"
#include "util/gdk_helpers.h"
#include "util/log.h"

struct wkline {
	struct wkline_dimensions dim;
	json_t *config;
	const char *position;
};
