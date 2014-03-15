#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include "util/dbus_helpers.h"

static struct widget_config {
	const char *name;
	int refresh_interval;
} widget_config_defaults = {
	.name = "BAT0",
	.refresh_interval = 20,
};
