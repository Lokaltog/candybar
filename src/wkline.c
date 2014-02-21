#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <webkit/webkit.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "wkline.h"
#include "config.h"

static int
get_intern_atom (xcb_connection_t *conn, char *atom) {
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply;

	cookie = xcb_intern_atom(conn, 0, strlen(atom), atom);
	reply = xcb_intern_atom_reply(conn, cookie, NULL);

	if (reply) {
		return reply->atom;
	}

	return -1;
}

static void
wk_notify_progress_cb (WebKitWebView* web_view, GParamSpec* pspec, GtkWidget* window) {
	float progress = webkit_web_view_get_progress(web_view);
	printf("Loading... %.0f%%\n", progress * 100);
	fflush(stdout);
}

static void
wk_notify_load_status_cb (WebKitWebView* web_view, GParamSpec* pspec, GtkWidget* window) {
	WebKitLoadStatus status = webkit_web_view_get_load_status(web_view);
	if (status == WEBKIT_LOAD_FINISHED) {
		printf("Page loaded!\n");
		fflush(stdout);
		webkit_web_view_execute_script(web_view, "wkInject({placeholder:'Text sent from wkline.c'})");
	}
}

static gboolean
wk_context_menu_cb (WebKitWebView* web_view, GtkWidget* window) {
	// Disable context menu
	return TRUE;
}

int
main (int argc, char *argv[]) {
	gchar *uri = (gchar *)(argc > 1 ? argv[1] : "about:blank");
	guint window_xid;

	GtkWindow *window;
	GtkLayout *layout;
	WebKitWebView *web_view;

	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	const int ATOM__NET_WM_STRUT_PARTIAL = get_intern_atom(conn, "_NET_WM_STRUT_PARTIAL");

	WklineDimensions dim = {screen->width_in_pixels, wkline_height};
	int strut_partial[12] = {0, 0, dim.h, 0, 0, 0, 0, 0, 0, dim.w, 0, 0};

	gtk_init(NULL, NULL);

	// GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout is used instead
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	// Set window dock properties
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
	g_signal_connect(web_view, "notify::progress", G_CALLBACK(wk_notify_progress_cb), web_view);
	g_signal_connect(web_view, "notify::load-status", G_CALLBACK(wk_notify_load_status_cb), web_view);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	printf("Opening URI '%s'...\n\n", uri);
	webkit_web_view_load_uri(web_view, uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	window_xid = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(window)));

	// Reserve space on top of root window with _NET_WM_STRUT_PARTIAL
	xcb_change_property(conn,
	                    XCB_PROP_MODE_REPLACE,
	                    window_xid,
	                    ATOM__NET_WM_STRUT_PARTIAL,
	                    XCB_ATOM_CARDINAL,
	                    32,
	                    sizeof(strut_partial), strut_partial);

	xcb_flush(conn);
	gtk_main();

	return 0;
}
