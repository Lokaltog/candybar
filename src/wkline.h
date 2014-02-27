#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <webkit/webkit.h>

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

static gboolean wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window);
void wklog (char const *format, ...);
static void wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);
