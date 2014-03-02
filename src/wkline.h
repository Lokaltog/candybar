#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <xcb/randr.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

typedef struct wk_display_info_t {
    int width;
    int offset;
} wk_display_info_t;

#define LENGTH(X) (sizeof X / sizeof X[0])
