#include "candybar.h"
#include "widgets.h"

static struct candybar *candybar = NULL;

static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	/* Disable context menu */
	return TRUE;
}

static void
parse_args (int argc, char *argv[], char **config_filename) {
	int opt;
	int int_arg;
	char *end;

	while ((opt = getopt(argc, argv, "c:h:m:p:t:")) != -1) {
		switch (opt) {
		case 'c':
			*config_filename = optarg;
			break;
		case 'h':
			int_arg = strtol(optarg, &end, 10);
			if (*end) {
				LOG_ERR("invalid value for 'height': %s", optarg);
				exit(EXIT_FAILURE);
			}
			candybar->height = int_arg;
			break;
		case 'p':
			candybar->position = !strcmp(optarg, "bottom")
			                   ? CANDYBAR_POSITION_BOTTOM : CANDYBAR_POSITION_TOP;
			break;
		case 'm':
			int_arg = strtol(optarg, &end, 10);
			if (*end) {
				LOG_ERR("invalid value for 'monitor': %s", optarg);
				exit(EXIT_FAILURE);
			}
			candybar->monitor = int_arg;
			break;
		case 't':
			candybar->theme_uri = optarg;
			break;
		}
	}
}

static void
wk_realize_handler (GtkWidget *window, gpointer user_data) {
	GdkAtom net_wm_strut_atom = gdk_atom_intern_static_string("_NET_WM_STRUT");
	GdkAtom net_wm_strut_partial_atom = gdk_atom_intern_static_string("_NET_WM_STRUT_PARTIAL");
	GdkAtom cardinal_atom = gdk_atom_intern_static_string("CARDINAL");
	GdkWindow *gdkw = gtk_widget_get_window(GTK_WIDGET(window));
	gulong strut[4] = { 0 };
	gulong strut_partial[12] = { 0 };

	bool supports_net_wm_strut = g_list_find(gdk_get_net_supported(),
	                                         net_wm_strut_atom) != NULL;
	bool supports_net_wm_strut_partial = g_list_find(gdk_get_net_supported(),
	                                                 net_wm_strut_partial_atom) != NULL;

	if (candybar->position == CANDYBAR_POSITION_TOP) {
		strut[2] = candybar->height;
		strut_partial[2] = candybar->height;
		strut_partial[9] = candybar->width;
	}
	else if (candybar->position == CANDYBAR_POSITION_BOTTOM) {
		strut[3] = candybar->height;
		strut_partial[3] = candybar->height;
		strut_partial[11] = candybar->width;
	}

	gdk_property_change(gdkw, net_wm_strut_atom, cardinal_atom,
	                    32, GDK_PROP_MODE_REPLACE, (guchar*)strut, LENGTH(strut));
	gdk_property_change(gdkw, net_wm_strut_partial_atom, cardinal_atom,
	                    32, GDK_PROP_MODE_REPLACE, (guchar*)strut_partial, LENGTH(strut_partial));

	if (!supports_net_wm_strut && !supports_net_wm_strut_partial) {
		/* only set override redirect if we're unable to reserve space
		   with _NET_WM_STRUT or _NET_WM_STRUT_PARTIAL */
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
	             "enable-accelerated-compositing", FALSE,
	             "enable-dns-prefetching", FALSE,
	             "enable-java-applet", FALSE,
	             "enable-plugins", FALSE,
	             "enable-universal-access-from-file-uris", TRUE,
	             "enable-webgl", FALSE,
	             "enable-xss-auditor", FALSE,
	             NULL);

	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	webkit_web_view_set_settings(web_view, web_view_settings);

	return web_view;
}

static void
signal_handler (int signal) {
	if ((signal == SIGTERM) || (signal == SIGINT) || (signal == SIGHUP)) {
		join_widget_threads(candybar);
		gtk_main_quit();
	}
	if (signal == SIGUSR1) {
		LOG_DEBUG("reloading theme");
		webkit_web_view_reload_bypass_cache(candybar->web_view);
	}
}

int
main (int argc, char *argv[]) {
	char *config_filename = NULL;
	GtkWindow *window;
	GtkLayout *layout;
	GdkScreen *screen;
	int monitors_num;
	GdkRectangle dest;
	WebKitWebView *web_view;
	struct sigaction sa;

	gtk_init(&argc, &argv);

	LOG_INFO("%s%s%s %s (%s %s)", ANSI_ESC_CYAN, ANSI_ESC_BOLD, PACKAGE, VERSION, __DATE__, __TIME__);

	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	/* GtkScrolledWindow fails to lock small heights (<25px), so a GtkLayout
	   is used instead */
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	layout = GTK_LAYOUT(gtk_layout_new(NULL, NULL));
	web_view = web_view_init();

	/* init candybar configuration */
	candybar = calloc(1, sizeof(struct candybar));
	candybar->height = -1; /* default value */
	candybar->monitor = -1; /* default value */
	candybar->web_view = web_view;
	candybar->efd = eventfd(0, 0);
	if (candybar->efd == -1) {
		LOG_ERR("could not create event file descriptor: %s", strerror(errno));
		goto config_err;
	}

	parse_args(argc, argv, &config_filename);

	candybar->config = get_config_json(config_filename);
	if (!candybar->config) {
		LOG_ERR("an error occured while loading the config file, aborting");
		goto config_err;
	}

	if (candybar->position == CANDYBAR_POSITION_UNKNOWN) {
		candybar->position = !strcmp(get_config_option_string(candybar->config, "position"), "bottom")
		                   ? CANDYBAR_POSITION_BOTTOM : CANDYBAR_POSITION_TOP;
	}
	if (candybar->monitor == -1) {
		candybar->monitor = get_config_option_integer(candybar->config, "monitor");
	}

	json_t *theme_config = get_config_option(candybar->config, "theme", false);
	candybar->theme_config = get_config_option(theme_config, "config", true);
	if (candybar->theme_uri == NULL) {
		candybar->theme_uri = get_config_option_string(theme_config, "uri");
	}

	/* get window size */
	screen = gtk_window_get_screen(window);
	monitors_num = gdk_screen_get_n_monitors(screen);
	if (monitors_num - 1 < candybar->monitor) {
		LOG_ERR("invalid monitor index '%i'", candybar->monitor);
		goto config_err;
	}
	gdk_screen_get_monitor_geometry(screen, candybar->monitor, &dest);

	candybar->width = dest.width;
	if (candybar->height == -1) {
		candybar->height = get_config_option_integer(candybar->config, "height");
	}

	/* set window properties */
	gtk_window_set_default_size(window, candybar->width, candybar->height);
	gtk_window_stick(window);
	gtk_window_set_decorated(window, FALSE);
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);
	gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
	gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_widget_set_size_request(GTK_WIDGET(web_view), candybar->width, candybar->height);

	gtk_container_add(GTK_CONTAINER(layout), GTK_WIDGET(web_view));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(layout));

	if (candybar->position == CANDYBAR_POSITION_TOP) {
		gtk_window_move(window, 0, 0);
	}
	else if (candybar->position == CANDYBAR_POSITION_BOTTOM) {
		gtk_window_move(window, 0, dest.height - candybar->height);
	}

	g_signal_connect(web_view, "context-menu", G_CALLBACK(wk_context_menu_cb), NULL);
	g_signal_connect(web_view, "window-object-cleared", G_CALLBACK(window_object_cleared_cb), candybar);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "realize", G_CALLBACK(wk_realize_handler), candybar);

	LOG_INFO("loading theme '%s'", candybar->theme_uri);
	webkit_web_view_load_uri(web_view, candybar->theme_uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	gtk_main();

	json_decref(candybar->config);
	json_decref(theme_config);
config_err:
	free(candybar);

	return 0;
}
