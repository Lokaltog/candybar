#include "widgets.h"
#include "battery.h"
#include "util/dbus_helpers.h"

static int
widget_battery_send_update (struct widget *widget, DBusGProxy *properties_proxy, char *pathbuf) {
	gdouble percentage;
	guint state;
	gint64 time_to_empty64, time_to_full64;

	proxy_double_value(&percentage, properties_proxy, pathbuf, "Percentage");
	proxy_uint_value(&state, properties_proxy, pathbuf, "State");
	proxy_int64_value(&time_to_empty64, properties_proxy, pathbuf, "TimeToEmpty");
	proxy_int64_value(&time_to_full64, properties_proxy, pathbuf, "TimeToFull");

	/* jansson fails with 64-bit integers, but the time left on the battery
	   should fit in a 32-bit integer */
	unsigned int time_to_empty = time_to_empty64 & 0xffffffff;
	unsigned int time_to_full = time_to_full64 & 0xffffffff;

	json_t *json_data_object = json_object();
	char *json_payload;

	json_object_set_new(json_data_object, "percentage", json_real(percentage));
	json_object_set_new(json_data_object, "state", json_integer(state));
	json_object_set_new(json_data_object, "time_to_empty", json_integer(time_to_empty));
	json_object_set_new(json_data_object, "time_to_full", json_integer(time_to_full));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	return 0;
}

static void
widget_cleanup (void *arg) {
	LOG_INFO("widget cleanup: battery");
	DBusGProxy **proxy_ref = arg;

	if (proxy_ref[0] != NULL) {
		g_object_unref(proxy_ref[0]);
	}
	if (proxy_ref[1] != NULL) {
		g_object_unref(proxy_ref[1]);
	}
}

void*
widget_init (struct widget *widget) {
	DBusGConnection *conn;
	DBusGProxy *proxy;
	DBusGProxy *properties_proxy;
	char pathbuf[128];
	GError *error = NULL;

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	sprintf(pathbuf, "/org/freedesktop/UPower/devices/battery_%s",
	        json_string_value(wkline_widget_get_config(widget, "name")));

	if (conn == NULL) {
		LOG_ERR("dbus: failed to open connection to bus: %s\n", error->message);
		g_error_free(error);

		return 0;
	}

	if ((proxy = dbus_g_proxy_new_for_name(conn,
	                                       "org.freedesktop.UPower",
	                                       pathbuf,
	                                       "org.freedesktop.UPower.Device.Properties")) == NULL) {
		LOG_ERR("dbus: failed to create proxy object");

		return 0;
	}

	if ((properties_proxy = dbus_g_proxy_new_from_proxy(proxy,
	                                                    "org.freedesktop.DBus.Properties",
	                                                    dbus_g_proxy_get_path(proxy))) == NULL) {
		g_object_unref(proxy);
		LOG_ERR("dbus: failed to create proxy object");

		return 0;
	}

	unsigned int state;
	if (!proxy_uint_value(&state, properties_proxy, pathbuf, "State")) {
		LOG_ERR("dbus: invalid battery");

		return 0;
	}

	DBusGProxy *proxy_ptr[2] = { proxy, properties_proxy };
	pthread_cleanup_push(widget_cleanup, proxy_ptr);
	for (;;) {
		widget_battery_send_update(widget, properties_proxy, pathbuf);

		sleep(20);
	}
	pthread_cleanup_pop(1);
}
