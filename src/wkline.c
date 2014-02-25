#include <ctype.h>
#include <fcntl.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <webkit/webkit.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <jansson.h>

#include "wkline.h"
#include "widgets.h"
#include "config.h"

thread_data_t thread_data;
WebKitWebView *web_view;

static bool
ewmh_get_active_window_name(xcb_ewmh_connection_t *ewmh, int screen_nbr, char *window_name) {
	xcb_window_t active_window;
	xcb_ewmh_get_utf8_strings_reply_t window_data;
	size_t len;

	if (! xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window_unchecked(ewmh, screen_nbr), &active_window, NULL)) {
		fprintf(stderr, "Found no active window\n");
		return false;
	}

	if (! xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name_unchecked(ewmh, active_window), &window_data, NULL)) {
		fprintf(stderr, "Could not read WM_NAME from active window\n");
		return false;
	}

	len = MIN(PROPERTY_MAX_LEN, window_data.strings_len + 1);
	memcpy(window_name, window_data.strings, len);
	window_name[len] = 0;

	xcb_ewmh_get_utf8_strings_reply_wipe(&window_data);

	return true;
}

static bool
ewmh_get_desktop_list(xcb_ewmh_connection_t *ewmh, int screen_nbr, desktop_t *desktops) {
	unsigned short i;
	uint32_t desktop_curr, desktop_len, client_desktop;
	xcb_ewmh_get_windows_reply_t clients;

	// get current desktop
	if (! xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop_unchecked(ewmh, screen_nbr), &desktop_curr, NULL)) {
		fprintf(stderr, "Could not get current desktop\n");
		return false;
	}

	// get desktop count
	if (! xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops_unchecked(ewmh, screen_nbr), &desktop_len, NULL)) {
		fprintf(stderr, "Could not get desktop count\n");
		return false;
	}

	for (i = 0; i < desktop_len; i++) {
		desktops[i].is_selected = i == desktop_curr;
		desktops[i].is_valid = true;
		desktops[i].clients_len = 0;
	}

	// get clients
	if (! xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list_unchecked(ewmh, screen_nbr), &clients, NULL)) {
		fprintf(stderr, "Could not get client list\n");
		return false;
	}

	for (i = 0; i < clients.windows_len; i++) {
		if (! xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop_unchecked(ewmh, clients.windows[i]), &client_desktop, NULL)) {
			continue;
		}
		desktops[client_desktop].clients_len++;
		// TODO check urgent hint and assign to desktop
	}

	xcb_ewmh_get_windows_reply_wipe(&clients);

	return true;
}

static gboolean
wk_web_view_inject (char *payload) {
	char script[4096];

	sprintf(script, "if(typeof wkInject!=='undefined'){try{wkInject(%s)}catch(e){console.log(e)}}else{console.log('Web page incompatible or not loaded yet')}", payload);

	webkit_web_view_execute_script(web_view, script);

	return FALSE; // only run once
}

static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	// Disable context menu
	return TRUE;
}

static void
wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window) {
	WebKitLoadStatus status = webkit_web_view_get_load_status(web_view);

	json_t *json_base_object;
	json_t *json_init_object;
	char *json_payload;

	if (status == WEBKIT_LOAD_FINISHED) {
		// send init data
		json_base_object = json_object();
		json_init_object = json_object();
		json_object_set_new(json_base_object, "event", json_string("init"));
		json_object_set_new(json_base_object, "data", json_init_object);
		json_object_set_new(json_init_object, "height", json_integer(wkline_height));
		json_payload = json_dumps(json_base_object, 0);
		g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);
	}
}

int
main (int argc, char *argv[]) {
	unsigned short i;
	int screen_nbr = 0;

	xcb_connection_t *conn = xcb_connect(NULL, &screen_nbr);
	if (xcb_connection_has_error(conn)) {
		fprintf(stderr, "Could not connect to display %s.\n", getenv("DISPLAY"));
		return 1;
	}

	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

	xcb_screen_t *screen = ewmh->screens[screen_nbr];
	ewmh_data_t ewmh_data = {screen_nbr, ewmh};
	wk_dimensions_t dim = {.w = screen->width_in_pixels, .h = wkline_height};

	// init window
	// TODO set command-line arguments
	gchar *uri = "file:///home/kim/projects/wkline-theme-default/webroot/index.html";
	guint window_xid;
	int strut_partial[12] = {0, 0, dim.h, 0, 0, 0, 0, 0, 0, dim.w, 0, 0};
	GtkWindow *window;
	GtkLayout *layout;

	gtk_init(&argc, &argv);

	// GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout is used instead
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	// set window dock properties
	gtk_window_move(window, 0, 0);
	gtk_window_resize(window, dim.w, dim.h);
	gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);
	gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_stick(window);

	gtk_widget_set_size_request(GTK_WIDGET(web_view), dim.w, dim.h);

	gtk_container_add(GTK_CONTAINER(layout), GTK_WIDGET(web_view));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(layout));

	g_signal_connect(web_view, "context-menu", G_CALLBACK(wk_context_menu_cb), web_view);
	g_signal_connect(web_view, "notify::load-status", G_CALLBACK(wk_notify_load_status_cb), web_view);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	fprintf(stderr, "Opening URI '%s'...\n\n", uri);
	webkit_web_view_load_uri(web_view, uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	// reserve space on top of root window with _NET_WM_STRUT_PARTIAL
	window_xid = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(window)));
	xcb_change_property(conn,
	                    XCB_PROP_MODE_REPLACE,
	                    window_xid,
	                    ewmh->_NET_WM_STRUT_PARTIAL,
	                    XCB_ATOM_CARDINAL,
	                    32,
	                    sizeof(strut_partial), strut_partial);
	xcb_flush(conn);

	// start widget threads
	thread_data.ewmh = &ewmh_data;

	for (i = 0; i < WIDGETS_LEN; i++) {
		fprintf(stderr, "Creating widget thread\n");

		g_thread_new("widget", (GThreadFunc)wkline_enabled_widgets[i], &thread_data);
	}

	gtk_main();

	free(ewmh);

	return 0;
}
