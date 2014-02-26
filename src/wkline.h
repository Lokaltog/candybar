#include <alsa/asoundlib.h>
#include <curl/curl.h>
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

#define PROPERTY_MAX_LEN 256
#define DESKTOP_MAX_LEN 10

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

typedef struct desktop_t {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
} desktop_t;

typedef struct ewmh_data_t {
	int screen_nbr;
	xcb_ewmh_connection_t *conn;
} ewmh_data_t;

typedef struct thread_data_t {
	desktop_t desktops[DESKTOP_MAX_LEN];
	char active_window_name[PROPERTY_MAX_LEN];
	ewmh_data_t *ewmh;
} thread_data_t;

static gboolean wk_web_view_inject (char *payload);
static gboolean wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window);
static void wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);
