#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <pthread.h>

#include "util/config.h"
#include "util/dbus_helpers.h"

static struct widget_config {
	const char *name;
} widget_config_defaults = {
	.name = "BAT0",
};
