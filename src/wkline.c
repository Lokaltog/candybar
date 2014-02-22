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

#include "wkline.h"
#include "config.h"

static void
fifo_monitor (gpointer data) {
	char buf[256];
	int num, fd;
	WklineThreadData *thread_data = (WklineThreadData *)data;

	if (mkfifo(wkline_fifo_path, 0666) < 0) {
		perror("mkfifo");
	}

	while (1) {
		if ((fd = open(wkline_fifo_path, O_RDONLY)) < 0) {
			perror("FIFO open");
		}
		do {
			if ((num = read(fd, buf, sizeof(buf))) < 0) {
				perror("FIFO read");
			}
			else {
				buf[num] = '\0';
				if (num > 0) {
					thread_data->buf = buf;
					g_idle_add((GSourceFunc)fifo_monitor_inject_data, data);
				}
			}
		} while (num > 0);

		close(fd);
	}
}

static gboolean
fifo_monitor_inject_data(gpointer data) {
	WklineThreadData *thread_data = (WklineThreadData *)data;

	web_view_inject(thread_data->web_view, thread_data->buf);

	return FALSE;
}

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
web_view_inject (WebKitWebView *web_view, char *payload) {
	char script[256];

	fprintf(stderr, "Injecting data into web view: %s\n", payload);
	sprintf(script, "try{wkInject(%s)}catch(e){console.log(e)}", payload);

	webkit_web_view_execute_script(web_view, script);
}

static gboolean
wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window) {
	// Disable context menu
	return TRUE;
}

static void
wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window) {
	char init_data[256];
	WebKitLoadStatus status = webkit_web_view_get_load_status(web_view);

	if (status == WEBKIT_LOAD_FINISHED) {
		sprintf(init_data, "['init',{height:%i}]", wkline_height);
		web_view_inject(web_view, init_data);
	}
}

int
main (int argc, char *argv[]) {
	gchar *uri = "about:blank";
	guint window_xid;
	int c;

	GtkWindow *window;
	GtkLayout *layout;
	WebKitWebView *web_view;

	while ((c = getopt(argc, argv, "u:")) != -1) {
		switch (c) {
		case 'u':
			uri = optarg;
			break;

		case '?':
			if (optopt == 'c') {
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint (optopt)) {
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			}
			else {
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			}
			return 1;

		default:
			abort ();
		}
	}

	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		fprintf(stderr, "Could not connect to display %s.\n", getenv("DISPLAY"));
		return 1;
	}
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	WklineThreadData thread_data;
	WklineDimensions dim = {screen->width_in_pixels, wkline_height};
	int strut_partial_atom = get_intern_atom(conn, "_NET_WM_STRUT_PARTIAL");
	int strut_partial[12] = {0, 0, dim.h, 0, 0, 0, 0, 0, 0, dim.w, 0, 0};

	// Add UID to fifo path
	sprintf(wkline_fifo_path, wkline_fifo_path, getuid());
	fprintf(stderr, "FIFO path: %s\n", wkline_fifo_path);

	gtk_init(&argc, &argv);

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
	g_signal_connect(web_view, "notify::load-status", G_CALLBACK(wk_notify_load_status_cb), web_view);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	fprintf(stderr, "Opening URI '%s'...\n\n", uri);
	webkit_web_view_load_uri(web_view, uri);

	gtk_widget_show_all(GTK_WIDGET(window));

	window_xid = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(window)));

	// Reserve space on top of root window with _NET_WM_STRUT_PARTIAL
	xcb_change_property(conn,
	                    XCB_PROP_MODE_REPLACE,
	                    window_xid,
	                    strut_partial_atom,
	                    XCB_ATOM_CARDINAL,
	                    32,
	                    sizeof(strut_partial), strut_partial);

	xcb_flush(conn);

	thread_data.web_view = web_view;

	// Launch fifo monitoring thread
	g_thread_new("FIFO monitor thread", (GThreadFunc)fifo_monitor, &thread_data);

	gtk_main();

	return 0;
}
