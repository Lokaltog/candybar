#include <stdlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

#define LENGTH(X) (sizeof X / sizeof X[0])
