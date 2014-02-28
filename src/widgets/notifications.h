#include <stdbool.h>
#include <dbus/dbus.h>
#include "src/config.h"

static void dbus_array_reply(DBusConnection *connection, DBusMessage* msg, char* array[]);
