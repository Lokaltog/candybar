#include "widgets.h"
#include "external_ip.h"

static int
widget_send_update (struct widget *widget, struct widget_config config) {
	char *json_payload;
	char *external_ip;
	json_t *json_data_object;

	external_ip = wkline_curl_request(config.address);

	json_data_object = json_object();
	json_object_set_new(json_data_object, "ip", json_string(external_ip));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	free(external_ip);

	return 0;
}

void*
widget_init (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget, "address", config.address);
	widget_init_config_integer(widget, "refresh_interval", config.refresh_interval);

	for (;;) {
		widget_send_update(widget, config);

		sleep(config.refresh_interval);
	}
}
