#include <unistd.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webkit/webkit.h>

struct wkline_dimensions {
	int w;
	int h;
};

struct wkline {
	struct wkline_dimensions dim;
	json_t *config;
	const char *position;
};
