#include "util/log.h"
#include "gdk_helpers.h"

GList*
gdk_get_net_supported () {
	GdkAtom actual_property_type;
	int actual_format;
	int actual_length;
	long *data = NULL;
	GList *list = NULL;
	int i;

	if (!gdk_property_get(gdk_screen_get_root_window(gdk_screen_get_default()),
	                      gdk_atom_intern("_NET_SUPPORTED", FALSE),
	                      gdk_atom_intern("ATOM", FALSE),
	                      0,
	                      G_MAXLONG,
	                      FALSE,
	                      &actual_property_type,
	                      &actual_format,
	                      &actual_length,
	                      (guchar**)&data)) {
		gchar *actual_property_type_name;
		LOG_ERR("unable to get _NET_SUPPORTED");
		actual_property_type_name = gdk_atom_name(actual_property_type);
		if (actual_property_type_name) {
			LOG_INFO("actual_property_type: %s", actual_property_type_name);
			g_free(actual_property_type_name);
		}

		return NULL;
	}

	/* Put all of the GdkAtoms into a GList to return */
	for (i = 0; i < actual_length / sizeof(long); i++) {
		list = g_list_append(list, (GdkAtom)data[i]);
	}

	g_free(data);

	return list;
}
