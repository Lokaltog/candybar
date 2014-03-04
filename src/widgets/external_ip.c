#include "widgets.h"
#include "external_ip.h"

static int
widget_external_ip_send_update (widget_data_t *widget_data) {
	char *external_ip = wkline_curl_request(wkline_widget_external_ip_address);

	json_t *json_data_object = json_object();
	char *json_payload;

	json_object_set_new(json_data_object, "ip", json_string(external_ip));

	json_payload = json_dumps(json_data_object, 0);

	widget_data->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget_data);
	json_decref(json_data_object);

	free(external_ip);

	return 0;
}

void
*widget_external_ip (widget_data_t *widget_data) {
	for (;;) {
		widget_external_ip_send_update(widget_data);

		sleep(600);
	}

	return 0;
}
