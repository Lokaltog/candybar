#include <stdlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webkit/webkit.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

struct wkline_t{
	struct wk_dimensions_t dim;
	json_t *config;
	const char *position;
};

#define LENGTH(X) (sizeof X / sizeof X[0])
