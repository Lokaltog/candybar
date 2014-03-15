#include "wkline.h"
#include "widgets.h"

#ifndef DEBUG /* only disable context menu in prod build */
static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	/* Disable context menu */
	return TRUE;
}

#endif

static void
parse_args (int argc, char *argv[], json_t *config) {
	int opt;
	int int_arg;
	char *end;

	while ((opt = getopt(argc, argv, "h:p:t:m:")) != -1) {
		switch (opt) {
		case 'h':
			int_arg = strtol(optarg, &end, 10);
			if (*end) {
				LOG_ERR("invalid value for 'height': %s", optarg);
				exit(EXIT_FAILURE);
			}
			json_object_set(config, "height", json_integer(int_arg));
			break;
		case 's':
			int_arg = strtol(optarg, &end, 10);
			if (*end) {
				LOG_ERR("invalid value for 'screen': %s", optarg);
				exit(EXIT_FAILURE);
			}
			json_object_set(config, "screen", json_integer(int_arg));
			break;
		case 't':
			json_object_set(config, "theme_uri", json_string(optarg));
			break;
		case 'p':
			json_object_set(config, "position", json_string(optarg));
			break;
		}
	}
}

static void
wk_realize_handler (GtkWidget *window, gpointer user_data) {
	struct wkline *wkline = user_data;
	GdkAtom net_wm_strut_atom = gdk_atom_intern("_NET_WM_STRUT", FALSE);
	GdkAtom net_wm_strut_partial_atom = gdk_atom_intern("_NET_WM_STRUT_PARTIAL", FALSE);
	GdkWindow *gdkw = gtk_widget_get_window(GTK_WIDGET(window));
	int strut[4] = { 0 };
	int strut_partial[12] = { 0 };

	bool supports_net_wm_strut = g_list_find(gdk_get_net_supported(),
	                                         net_wm_strut_atom) != NULL;
	bool supports_net_wm_strut_partial = g_list_find(gdk_get_net_supported(),
	                                                 net_wm_strut_partial_atom) != NULL;

	if (wkline->position == WKLINE_POSITION_TOP) {
		strut[2] = wkline->height;
		strut_partial[2] = wkline->height;
		strut_partial[9] = wkline->width;
	}
	else if (wkline->position == WKLINE_POSITION_BOTTOM) {
		strut[3] = wkline->height;
		strut_partial[3] = wkline->height;
		strut_partial[11] = wkline->width;
	}

	if (supports_net_wm_strut) {
		gdk_property_change(gdkw, net_wm_strut_atom, gdk_atom_intern("CARDINAL", FALSE),
		                    32, GDK_PROP_MODE_REPLACE, (guchar*)strut, LENGTH(strut));
	}
	else if (supports_net_wm_strut_partial) {
		gdk_property_change(gdkw, net_wm_strut_partial_atom, gdk_atom_intern("CARDINAL", FALSE),
		                    32, GDK_PROP_MODE_REPLACE, (guchar*)strut_partial, LENGTH(strut_partial));
	}
	else {
		/* only set override redirect if we're unable to reserve space
		   with _NET_WM_STRUT */
		gdk_window_set_override_redirect(gdkw, TRUE);
	}
}

static WebKitWebView*
web_view_init () {
	WebKitWebView *web_view;
	WebKitWebSettings *web_view_settings;
	WebKitWebPluginDatabase *database;
	GSList *plugin_list, *p;
	WebKitWebPlugin *plugin;

	/* disable all plugins */
	database = webkit_get_web_plugin_database();
	plugin_list = webkit_web_plugin_database_get_plugins(database);
	for (p = plugin_list; p; p = p->next) {
		plugin = (WebKitWebPlugin*)p->data;
		webkit_web_plugin_set_enabled(plugin, FALSE);
	}
	webkit_web_plugin_database_refresh(database);
	webkit_web_plugin_database_plugins_list_free(plugin_list);

	/* set webview settings */
	web_view_settings = webkit_web_settings_new();
	g_object_set(G_OBJECT(web_view_settings),
	             "enable-accelerated-compositing", TRUE,
	             "enable-css-shaders", TRUE,
	             "enable-dns-prefetching", FALSE,
	             "enable-java-applet", FALSE,
	             "enable-plugins", FALSE,
	             "enable-universal-access-from-file-uris", TRUE,
	             "enable-webgl", TRUE,
	             "enable-xss-auditor", FALSE,
	             NULL);

	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	webkit_web_view_set_settings(web_view, web_view_settings);

	return web_view;
}

int
main (int argc, char *argv[]) {
	struct wkline *wkline = NULL;
	GtkWindow *window;
	GtkLayout *layout;
	GdkScreen *screen;
	GdkRectangle dest;
	WebKitWebView *web_view;

	gtk_init(&argc, &argv);

	LOG_INFO("%s%s%s %s (%s %s)", ANSI_ESC_CYAN, ANSI_ESC_BOLD, PACKAGE, VERSION, __DATE__, __TIME__);

	signal(SIGTERM, handle_interrupt);
	signal(SIGINT, handle_interrupt);
	signal(SIGHUP, handle_interrupt);

	/* GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout
	   is used instead */
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = web_view_init();

	/* init wkline configuration */
	wkline = malloc(sizeof(struct wkline));
	wkline->config = load_config_file();
	if (!wkline->config) {
		LOG_ERR("config file not found.");
		goto config_err;
	}
	parse_args(argc, argv, wkline->config);

	wkline->position = !strcmp(json_string_value(wkline_get_config(wkline, "position")), "bottom")
	                   ? WKLINE_POSITION_BOTTOM : WKLINE_POSITION_TOP;
	wkline->screen = json_integer_value(wkline_get_config(wkline, "screen"));
	wkline->theme_uri = json_string_value(wkline_get_config(wkline, "theme_uri"));
	wkline->web_view = web_view;

	/* get window size */
	screen = gtk_window_get_screen(window);
	gdk_screen_get_monitor_geometry(screen, wkline->screen, &dest);

	wkline->width = dest.width;
	wkline->height = json_integer_value(wkline_get_config(wkline, "height"));

	/* set window properties */
	gtk_window_set_default_size(window, wkline->width, wkline->height);
	gtk_window_stick(window);
	gtk_window_set_decorated(window, FALSE);
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);
	gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
	gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_widget_set_size_request(GTK_WIDGET(web_view), wkline->width, wkline->height);

	gtk_container_add(GTK_CONTAINER(layout), GTK_WIDGET(web_view));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(layout));

	if (wkline->position == WKLINE_POSITION_TOP) {
		gtk_window_move(window, dest.x, 0);
	}
	else if (wkline->position == WKLINE_POSITION_BOTTOM) {
		gtk_window_move(window, dest.x, dest.y - wkline->height);
	}

#ifndef DEBUG /* only disable context menu in prod build */
	g_signal_connect(web_view, "context-menu", G_CALLBACK(wk_context_menu_cb), NULL);
#endif
	g_signal_connect(web_view, "window-object-cleared", G_CALLBACK(window_object_cleared_cb), wkline);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "realize", G_CALLBACK(wk_realize_handler), wkline);

	LOG_INFO("loading theme '%s'", wkline->theme_uri);
	webkit_web_view_load_uri(web_view, wkline->theme_uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	gtk_main();

	json_decref(wkline->config);
config_err:
	free(wkline);

	return 0;
}
