#include "widgets.h"
#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

char*
widget_type () {
	return "desktops";
}

void
widget_callback (i3ipcConnection *conn, i3ipcWorkspaceEvent *event, gpointer user_data) {
	LOG_DEBUG("test");
}

void*
widget_main (struct widget *widget) {
	i3ipcConnection *conn = i3ipc_connection_new(NULL, NULL);

	void (*callback) (i3ipcConnection *, i3ipcWorkspaceEvent *, gpointer);
	callback = widget_callback;

	GClosure *closure = g_cclosure_new(G_CALLBACK(callback), NULL, NULL);
	i3ipc_connection_on(conn, (const gchar*)"workspace", closure, NULL);

	widget_epoll_init(widget);
	while (true) {
		widget_epoll_wait_goto(widget, 0, cleanup);
	}
	
cleanup:
	g_object_unref(conn);

	return 0;
}
