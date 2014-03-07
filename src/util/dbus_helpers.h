#include <stdbool.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "util/log.h"

gboolean proxy_double_value (gdouble *value, DBusGProxy *properties_proxy, char *path, char *property);
gboolean proxy_uint64_value (guint64 *value, DBusGProxy *properties_proxy, char *path, char *property);
gboolean proxy_uint_value (guint *value, DBusGProxy *properties_proxy, char *path, char *property);
gboolean proxy_int64_value (gint64 *value, DBusGProxy *properties_proxy, char *path, char *property);
gboolean proxy_int_value (gint *value, DBusGProxy *properties_proxy, char *path, char *property);
