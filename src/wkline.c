#include "config.h"
#include "wkline.h"
#include "util/log.h"
#include "widgets/widgets.h"

static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	// Disable context menu
	return TRUE;
}

static void
wk_realize_handler(GtkWidget *window, gpointer user_data){
	wk_dimensions_t *dim = user_data;
	GdkAtom atom;
	GdkWindow *gdkw;
	long vals[4] = {0,0,dim->h,0};

	atom = gdk_atom_intern ("_NET_WM_STRUT", FALSE);

	gdkw = gtk_widget_get_window(GTK_WIDGET(window));
	gdk_property_change(gdkw, atom, gdk_atom_intern("CARDINAL", FALSE),
	                    32, GDK_PROP_MODE_REPLACE, (guchar *)vals, LENGTH(vals));
}

int
main (int argc, char *argv[]) {
	GtkWindow *window;
	GtkLayout *layout;
	GdkScreen *screen;
	GdkRectangle dest;
	gint monitor_num;
	wk_dimensions_t dim;
	WebKitWebView *web_view;

	gtk_init(&argc, &argv);

	// GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout is used instead
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	// get window size
	screen = gtk_window_get_screen(window);
	gdk_screen_get_monitor_geometry (screen, wkline_monitor, &dest);
	dim.w = dest.width;
	dim.h = wkline_height; /* defined in config.h */

	// set window dock properties
	gtk_window_move(window, dest.x, 0);
	gtk_window_set_default_size(window, dim.w, dim.h);
	gtk_window_stick(window);
	gtk_window_set_decorated(window, FALSE);
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);
	gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
	gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);

	gtk_widget_set_size_request(GTK_WIDGET(web_view), dim.w, dim.h);

	gtk_container_add(GTK_CONTAINER(layout), GTK_WIDGET(web_view));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(layout));

#ifndef DEBUG // only disable context menu in prod build
	g_signal_connect(web_view, "context-menu", G_CALLBACK(wk_context_menu_cb), NULL);
#endif
	g_signal_connect(web_view, "window-object-cleared", G_CALLBACK(window_object_cleared_cb), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "realize", G_CALLBACK(wk_realize_handler), &dim);

	wklog("Opening URI '%s'", wkline_theme_uri);
	webkit_web_view_load_uri(web_view, wkline_theme_uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	gtk_main();
	return 0;
}
