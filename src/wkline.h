#include <stdlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webkit/webkit.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

struct wkline_dimensions {
	int w;
	int h;
};

struct wkline {
	struct wkline_dimensions dim;
	json_t *config;
	const char *position;
};
