#include "dbus_helpers.h"

static gboolean
proxy_property_value (DBusGProxy *properties_proxy, char *path, char *property, GValue *get_value, GError **error)
{
	return dbus_g_proxy_call(properties_proxy, "Get", error,
	                         G_TYPE_STRING, path,
	                         G_TYPE_STRING, property,
	                         G_TYPE_INVALID,
	                         G_TYPE_VALUE, get_value,
	                         G_TYPE_INVALID);
}

gboolean
proxy_double_value (gdouble *value, DBusGProxy *properties_proxy, char *path, char *property) {
	GError *error = NULL;
	GValue get_value = {0, };

	if (! proxy_property_value(properties_proxy, path, property, &get_value, &error)) {
		wklog("dbus error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	*value = g_value_get_double(&get_value);
	g_value_unset(&get_value);
	return TRUE;
}

gboolean
proxy_uint64_value (guint64 *value, DBusGProxy *properties_proxy, char *path, char *property) {
	GError *error = NULL;
	GValue get_value = {0, };

	if (! proxy_property_value(properties_proxy, path, property, &get_value, &error)) {
		wklog("dbus error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	*value = g_value_get_uint64(&get_value);
	g_value_unset(&get_value);
	return TRUE;
}

gboolean
proxy_uint_value (guint *value, DBusGProxy *properties_proxy, char *path, char *property) {
	GError *error = NULL;
	GValue get_value = {0, };

	if (! proxy_property_value(properties_proxy, path, property, &get_value, &error)) {
		wklog("dbus error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	*value = g_value_get_uint(&get_value);
	g_value_unset(&get_value);
	return TRUE;
}

gboolean
proxy_int64_value (gint64 *value, DBusGProxy *properties_proxy, char *path, char *property) {
	GError *error = NULL;
	GValue get_value = {0, };

	if (! proxy_property_value(properties_proxy, path, property, &get_value, &error)) {
		wklog("dbus error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	*value = g_value_get_int64(&get_value);
	g_value_unset(&get_value);
	return TRUE;
}

gboolean
proxy_int_value (gint *value, DBusGProxy *properties_proxy, char *path, char *property) {
	GError *error = NULL;
	GValue get_value = {0, };

	if (! proxy_property_value(properties_proxy, path, property, &get_value, &error)) {
		wklog("dbus error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	*value = g_value_get_int(&get_value);
	g_value_unset(&get_value);
	return TRUE;
}
