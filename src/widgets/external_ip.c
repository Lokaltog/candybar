#include "widgets.h"
#include "external_ip.h"

static int
widget_update (struct widget *widget, struct widget_config config) {
	char *external_ip;

	external_ip = wkline_curl_request(config.address);

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "ip", json_string(external_ip));

	widget_send_update(json_data_object, widget);

	free(external_ip);

	return 0;
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "address", config.address);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	for (;;) {
		widget_update(widget, config);

		sleep(config.refresh_interval);
	}
}
