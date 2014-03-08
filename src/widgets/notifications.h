#include <dbus/dbus.h>
#include <pthread.h>
#include <stdbool.h>

static void dbus_array_reply (DBusConnection *connection, DBusMessage *msg, char *array[]);
