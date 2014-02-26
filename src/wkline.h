#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webkit/webkit.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

typedef struct thread_data_t {
	int screen_nbr;
	xcb_ewmh_connection_t *ewmh;
} thread_data_t;

static gboolean wk_web_view_inject (char *payload);
static gboolean wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window);
void wklog (char const *format, ...);
static void wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);

extern thread_data_t thread_data;
