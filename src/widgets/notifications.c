#include "widgets.h"
#include "notifications.h"

static char *server_info[] = { "wkline", "Lokaltog", "0.1", "1.2", NULL };
static char *server_capabilities[] = { "body", NULL };

static void
dbus_array_reply (DBusConnection *connection, DBusMessage *msg, char *array[]) {
	DBusMessage *reply = dbus_message_new_method_return(msg);
	DBusMessageIter args;

	bool success = true;
	dbus_message_iter_init_append(reply, &args);
	int i;
	for (i = 0; array[i] != 0; i++) {
		if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &array[i])) {
			success = false;
		}
	}
	if (success && dbus_connection_send(connection, reply, NULL)) {
		dbus_message_unref(reply);
	}
}

static int
widget_send_update (struct widget *widget, DBusConnection *connection, DBusMessage *msg) {
	unsigned short i;
	DBusMessage *reply;
	DBusMessageIter args;
	const char *appname;
	const char *summary;
	const char *body;
	dbus_uint32_t nid = 0;
	dbus_int32_t expires = -1;
	void *to_fill = NULL;

	dbus_message_iter_init(msg, &args);
	for (i = 0; i < 8; i++) {
		switch (i) {
		case 0:
			to_fill = &appname;
			break;
		case 1:
			to_fill = &nid;
			break;
		case 3:
			to_fill = &summary;
			break;
		case 4:
			to_fill = &body;
			break;
		case 7:
			to_fill = &expires;
			break;
		default:
			to_fill = NULL;
			break;
		}
		if (to_fill) {
			dbus_message_iter_get_basic(&args, to_fill);
		}
		dbus_message_iter_next(&args);
	}

	/* send reply */
	reply = dbus_message_new_method_return(msg);
	dbus_message_iter_init_append(reply, &args);
	if (dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &nid)) {
		dbus_connection_send(connection, reply, NULL);
	}
	dbus_message_unref(reply);

	json_t *json_data_object = json_object();
	char *json_payload;

	json_object_set_new(json_data_object, "appname", json_string(appname));
	json_object_set_new(json_data_object, "summary", json_string(summary));
	json_object_set_new(json_data_object, "body", json_string(body));
	json_object_set_new(json_data_object, "expires", json_integer(expires));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	return 0;
}

static void
widget_cleanup (void *arg) {
	LOG_INFO("widget cleanup: notifications");

	void **cleanup_data = arg;

	dbus_connection_unref(cleanup_data[0]);
	free(arg);
}

void*
widget_init (struct widget *widget) {
	DBusConnection *connection;
	DBusError dbus_error;
	DBusError *err = &dbus_error;
	DBusMessage *msg;
	int server_result;

	dbus_error_init(err);
	connection = dbus_bus_get(DBUS_BUS_SESSION, err);
	if (dbus_error_is_set(err)) {
		LOG_ERR("dbus connection error: %s", err->message);
		dbus_error_free(err);

		return 0;
	}
	if (!connection) {
		LOG_ERR("dbus: no connection");

		return 0;
	}

	server_result = dbus_bus_request_name(connection, "org.freedesktop.Notifications", DBUS_NAME_FLAG_REPLACE_EXISTING, err);
	if (dbus_error_is_set(err)) {
		LOG_ERR("dbus error: %s", err->message);
		dbus_error_free(err);

		return 0;
	}
	if (server_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		LOG_WARN("dbus: a notification daemon is already running");

		return 0;
	}
	dbus_error_free(err);

	void **cleanup_data = malloc(sizeof(void*) * 1);
	cleanup_data[0] = connection;

	pthread_cleanup_push(widget_cleanup, cleanup_data);
	for (;;) {
		dbus_connection_read_write(connection, -1);

		while ((msg = dbus_connection_pop_message(connection)) != NULL) {
			if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify")) {
				if (widget_send_update(widget, connection, msg) != 0) {
					LOG_ERR("dbus: error while handling notification");
					break;
				}
			}
			else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "GetServerInformation")) {
				dbus_array_reply(connection, msg, server_info);
			}
			else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "GetCapabilities")) {
				dbus_array_reply(connection, msg, server_capabilities);
			}

			dbus_message_unref(msg);
			dbus_connection_flush(connection);
		}
	}
	pthread_cleanup_pop(1);
}
