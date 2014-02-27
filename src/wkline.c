#include "config.h"
#include "wkline.h"
#include "util/curl.h"
#include "util/log.h"
#include "widgets/widgets.h"

thread_data_t thread_data;
WebKitWebView *web_view;
GThread *widget_threads[WIDGETS_LEN];

gboolean
update_widget (widget_data_t *widget_data) {
	char *script_template = "if(typeof widgets!=='undefined'){try{widgets.update('%s',%s)}catch(e){console.log('Could not update widget: '+e)}}";
	char script[4096];

#ifdef DEBUG_JSON
	wklog("Updating widget %s: %s", widget_data->widget, widget_data->data);
#endif
	sprintf(script, script_template, widget_data->widget, widget_data->data);

	webkit_web_view_execute_script(web_view, script);
	free(widget_data);

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

	if (status == WEBKIT_LOAD_FINISHED) {
		unsigned short i;
		for (i = 0; i < WIDGETS_LEN; i++) {
			// FIXME this is pretty bad, it should probably join and recreate the threads instead
			if (! widget_threads[i] && wkline_widgets[i]) {
				wklog("Creating widget thread");
				widget_threads[i] = g_thread_new("widget", (GThreadFunc)wkline_widgets[i], NULL);
			}
		}
	}
}

int
main (int argc, char *argv[]) {
	int screen_nbr = 0;

	xcb_connection_t *conn = xcb_connect(NULL, &screen_nbr);
	if (xcb_connection_has_error(conn)) {
		wklog("Could not connect to display %s.", getenv("DISPLAY"));
		return 1;
	}

	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

	xcb_screen_t *screen = ewmh->screens[screen_nbr];
	wk_dimensions_t dim = {.w = screen->width_in_pixels, .h = wkline_height};

	// init window
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

	wklog("Opening URI '%s'", wkline_theme_uri);
	webkit_web_view_load_uri(web_view, wkline_theme_uri);

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

	thread_data.screen_nbr = screen_nbr;
	thread_data.ewmh = ewmh;

	gtk_main();

	free(ewmh);

	return 0;
}
