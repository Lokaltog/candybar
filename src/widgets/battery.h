#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include "util/dbus_helpers.h"

static struct widget_config {
	const char *dbus_path;
	int refresh_interval;
} widget_config_defaults = {
	.dbus_path = "/org/freedesktop/UPower/devices/battery_BAT0",
	.refresh_interval = 20,
};
