#include "widgets.h"
#include "battery.h"

static int
widget_update (struct widget *widget, DBusGProxy *properties_proxy, char *dbus_path) {
	gdouble percentage = 0;
	guint state = 0;
	gint64 time_to_empty64 = 0, time_to_full64 = 0;

	proxy_double_value(&percentage, properties_proxy, dbus_path, "Percentage");
	proxy_uint_value(&state, properties_proxy, dbus_path, "State");
	proxy_int64_value(&time_to_empty64, properties_proxy, dbus_path, "TimeToEmpty");
	proxy_int64_value(&time_to_full64, properties_proxy, dbus_path, "TimeToFull");

	/* jansson fails with 64-bit integers, but the time left on the battery
	   should fit in a 32-bit integer */
	unsigned int time_to_empty = time_to_empty64 & 0xffffffff;
	unsigned int time_to_full = time_to_full64 & 0xffffffff;

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "percentage", json_real(percentage));
	json_object_set_new(json_data_object, "state", json_integer(state));
	json_object_set_new(json_data_object, "time_to_empty", json_integer(time_to_empty));
	json_object_set_new(json_data_object, "time_to_full", json_integer(time_to_full));

	widget_send_update(json_data_object, widget);

	return 0;
}

static void
widget_cleanup (void *arg) {
	LOG_DEBUG("cleanup");

	void **cleanup_data = arg;

	if (cleanup_data[0] != NULL) {
		g_object_unref(cleanup_data[0]);
	}
	if (cleanup_data[1] != NULL) {
		g_object_unref(cleanup_data[1]);
	}
	if (cleanup_data[2] != NULL) {
		free(cleanup_data[2]);
	}
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "name", config.name);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	DBusGConnection *conn = NULL;
	DBusGProxy *proxy = NULL;
	DBusGProxy *properties_proxy = NULL;
	char *dbus_path = NULL;
	char *dbus_path_template = "/org/freedesktop/UPower/devices/battery_%s";
	int dbus_path_length = 0;
	GError *error = NULL;

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	dbus_path_length = snprintf(NULL, 0, dbus_path_template, config.name);
	dbus_path = malloc(dbus_path_length + 1);
	snprintf(dbus_path, dbus_path_length + 1, dbus_path_template, config.name);

	if (conn == NULL) {
		LOG_ERR("dbus: failed to open connection to bus: %s\n", error->message);
		goto cleanup;
	}

	proxy = dbus_g_proxy_new_for_name(conn, "org.freedesktop.UPower", dbus_path,
	                                  "org.freedesktop.UPower.Device.Properties");
	if (proxy == NULL) {
		LOG_ERR("dbus: failed to create proxy object");
		goto cleanup;
	}
	properties_proxy = dbus_g_proxy_new_from_proxy(proxy, "org.freedesktop.DBus.Properties",
	                                               dbus_g_proxy_get_path(proxy));
	if (properties_proxy == NULL) {
		LOG_ERR("dbus: failed to create proxy object");
		goto cleanup;
	}

	unsigned int state;
	if (!proxy_uint_value(&state, properties_proxy, dbus_path, "State")) {
		LOG_ERR("dbus: invalid battery");
		if (proxy != NULL) {
			g_object_unref(proxy);
		}
		if (properties_proxy != NULL) {
			g_object_unref(properties_proxy);
		}
		goto cleanup;
	}

	void *cleanup_data[] = { proxy, properties_proxy, dbus_path };
	pthread_cleanup_push(widget_cleanup, &cleanup_data);
	for (;;) {
		widget_update(widget, properties_proxy, dbus_path);

		sleep(config.refresh_interval);
	}
	pthread_cleanup_pop(1);

cleanup:
	if (error != NULL) {
		g_error_free(error);
	}
	if (dbus_path != NULL) {
		free(dbus_path);
	}

	return 0;
}
