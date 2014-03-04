#include "widgets.h"
#include "external_ip.h"
#include "util/load_config.h"

static int
widget_external_ip_send_update (struct wkline_widget_t *widget) {
	char *json_payload;
	char *external_ip;
	json_t *json_data_object;

	external_ip = wkline_curl_request(json_string_value(wkline_widget_get_config(widget, "address")));
	json_data_object = json_object();
	json_object_set_new(json_data_object, "ip", json_string(external_ip));

	json_payload = json_dumps(json_data_object, 0);

	widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
	widget_data->widget = "external_ip";
	widget_data->data = json_payload;
	g_idle_add((GSourceFunc)update_widget, widget_data);

	free(external_ip);

	return 0;
}

void
*widget_external_ip (struct wkline_widget_t *widget) {
	for (;;) {
		widget_external_ip_send_update(widget);

		sleep(600);
	}

	return 0;
}
