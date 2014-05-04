#include "widgets.h"
#include <glib.h>
#include <glib/gprintf.h>
#include <jansson.h>
#include <i3ipc-glib/i3ipc-glib.h>

char*
widget_type () {
	return "desktops";
}

gint
workspace_comparator (gconstpointer pointer_a, gconstpointer pointer_b) {
	i3ipcWorkspaceReply *a = (i3ipcWorkspaceReply*)pointer_a;
	i3ipcWorkspaceReply *b = (i3ipcWorkspaceReply*)pointer_b;

	if (a->num < b->num) {
		return -1;
	}
	else if (a->num > b->num) {
		return 1;
	}
	else {
		return 0;
	}
}

json_t*
format_workspaces (i3ipcConnection *conn) {
	json_t *json_data_object = json_object();
	json_t *json_desktops_array = json_array();
	json_object_set_new(json_data_object, "desktops", json_desktops_array);

	GSList *workspaces = i3ipc_connection_get_workspaces(conn, NULL);
	workspaces = g_slist_sort(workspaces, (GCompareFunc)workspace_comparator);
	for (unsigned char i = 0; i < g_slist_length(workspaces); i++) {
		GSList *workspace = g_slist_nth(workspaces, i);
		i3ipcWorkspaceReply *data = workspace->data;

		json_t *json_desktop = json_object();
		json_object_set_new(json_desktop, "name", json_string(data->name));
		json_object_set_new(json_desktop, "clients_len", json_integer(i));
		json_object_set_new(json_desktop, "is_urgent", json_boolean(data->urgent));
		json_array_append_new(json_desktops_array, json_desktop);

		if (data->focused) {
			json_object_set_new(json_data_object, "current_desktop", json_integer(i));
		}
	}

	return json_data_object;
}

void
workspace_callback (i3ipcConnection *conn, i3ipcWorkspaceEvent *event, gpointer user_data) {
	struct widget *widget = (struct widget*)user_data;
	json_t *json_data_object = format_workspaces(conn);
	char *json_str = strdup(json_dumps(json_data_object, 0));
	struct js_callback_arg arg = widget_data_arg_string(json_str);
	struct js_callback_data data = { .widget = widget, .args = &arg, 1 };
	web_view_callback(&data);
	free(json_str);
}

void*
widget_main (struct widget *widget) {
	i3ipcConnection *conn = i3ipc_connection_new(NULL, NULL);

	json_t *json_data_object = format_workspaces(conn);
	char *json_str = strdup(json_dumps(json_data_object, 0));
	widget_data_callback(widget, widget_data_arg_string(json_str));
	free(json_str);

	void (*callback)(i3ipcConnection*, i3ipcWorkspaceEvent*, gpointer);
	callback = workspace_callback;

	GClosure *closure = g_cclosure_new(G_CALLBACK(callback), widget, NULL);
	i3ipc_connection_on(conn, (const gchar*)"workspace", closure, NULL);

	widget_epoll_init(widget);
	while (true) {
		widget_epoll_wait_goto(widget, 0, cleanup);
	}

cleanup:
	g_object_unref(conn);

	return 0;
}
