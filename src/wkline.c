#include "util/wkconfig.h"
#include "util/log.h"
#include "wkline.h"
#include "widgets.h"
#include "config.h"

#ifndef DEBUG /* only disable context menu in prod build */
static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	/* Disable context menu */
	return TRUE;
}
#endif

static void
wk_realize_handler (GtkWidget *window, gpointer user_data) {
	struct wkline *wkline = user_data;
	GdkAtom atom;
	GdkWindow *gdkw;
	long vals[4];

	vals[0] = 0;
	vals[1] = 0;
	if (!strcmp(wkline->position, "top")) {
		vals[2] = wkline->dim.h;
		vals[3] = 0;
	}
	else if (!strcmp(wkline->position, "bottom")) {
		vals[2] = 0;
		vals[3] = wkline->dim.h;
	}

	atom = gdk_atom_intern("_NET_WM_STRUT", FALSE);

	gdkw = gtk_widget_get_window(GTK_WIDGET(window));
	gdk_property_change(gdkw, atom, gdk_atom_intern("CARDINAL", FALSE),
	                    32, GDK_PROP_MODE_REPLACE, (guchar*)vals, LENGTH(vals));
}

int
main (int argc, char *argv[]) {
	struct wkline *wkline;
	GtkWindow *window;
	GtkLayout *layout;
	GdkScreen *screen;
	GdkRectangle dest;
	WebKitWebView *web_view;
	const char *wkline_theme_uri;

	gtk_init(&argc, &argv);

	wklog("%s", VERSION);
	wklog("built on %s", BUILD_DATE);

	wkline = malloc(sizeof(struct wkline));
	wkline->config = load_config_file();
	if (!wkline->config) {
		wklog("Error config file not found.");
		goto config_err;
	}

	signal(SIGTERM, handle_interrupt);
	signal(SIGINT, handle_interrupt);
	signal(SIGHUP, handle_interrupt);

	wkline->position = json_string_value(wkline_get_config(wkline, "position"));

	/* GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout
	   is used instead */
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	/* get window size */
	screen = gtk_window_get_screen(window);
	gdk_screen_get_monitor_geometry(screen,
	                                json_integer_value(wkline_get_config(wkline, "monitor")),
	                                &dest);
	wkline->dim.w = dest.width;
	wkline->dim.h = json_integer_value(wkline_get_config(wkline, "height"));

	/* set window dock properties */
	if (!strcmp(wkline->position, "top")) {
		gtk_window_move(window, dest.x, 0);
	}
	else if (!strcmp(wkline->position, "bottom")) {
		gtk_window_move(window, dest.x, dest.y - wkline->dim.h);
	}

	gtk_window_set_default_size(window, wkline->dim.w, wkline->dim.h);
	gtk_window_stick(window);
	gtk_window_set_decorated(window, FALSE);
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);
	gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
	gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);

	gtk_widget_set_size_request(GTK_WIDGET(web_view), wkline->dim.w, wkline->dim.h);

	gtk_container_add(GTK_CONTAINER(layout), GTK_WIDGET(web_view));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(layout));

#ifndef DEBUG /* only disable context menu in prod build */
	g_signal_connect(web_view, "context-menu", G_CALLBACK(wk_context_menu_cb), NULL);
#endif
	g_signal_connect(web_view, "window-object-cleared", G_CALLBACK(window_object_cleared_cb), wkline->config);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "realize", G_CALLBACK(wk_realize_handler), wkline);

	wkline_theme_uri = json_string_value(wkline_get_config(wkline, "theme_uri"));
	wklog("Opening URI '%s'", wkline_theme_uri);
	webkit_web_view_load_uri(web_view, wkline_theme_uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	gtk_main();

	json_decref(wkline->config);
config_err:
	free(wkline);

	return 0;
}
