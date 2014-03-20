#include "widgets.h"
#include "battery.h"

static int
widget_update (struct widget *widget, DBusGProxy *properties_proxy, char *dbus_path) {
	gdouble percentage = 0;
	guint state = 0;
	gint64 time_to_empty = 0, time_to_full = 0;

	proxy_double_value(&percentage, properties_proxy, dbus_path, "Percentage");
	proxy_uint_value(&state, properties_proxy, dbus_path, "State");
	proxy_int64_value(&time_to_empty, properties_proxy, dbus_path, "TimeToEmpty");
	proxy_int64_value(&time_to_full, properties_proxy, dbus_path, "TimeToFull");

	widget_data_callback(widget,
	                     widget_data_arg_number(percentage),
	                     widget_data_arg_number(state),
	                     widget_data_arg_number(time_to_empty),
	                     widget_data_arg_number(time_to_full));

	return 0;
}

void*
widget_init (struct widget *widget) {
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

	widget_epoll_init(widget);
	while (true) {
		widget_update(widget, properties_proxy, dbus_path);
		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:
	if (error != NULL) {
		g_error_free(error);
	}
	if (proxy != NULL) {
		g_object_unref(proxy);
	}
	if (properties_proxy != NULL) {
		g_object_unref(properties_proxy);
	}
	if (dbus_path != NULL) {
		free(dbus_path);
	}

	return 0;
}
