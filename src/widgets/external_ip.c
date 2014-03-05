#include "widgets.h"
#include "external_ip.h"

static int
widget_external_ip_send_update (struct widget *widget) {
	char *json_payload;
	char *external_ip;
	json_t *json_data_object;

	external_ip = wkline_curl_request(json_string_value(wkline_widget_get_config(widget, "address")));
	json_data_object = json_object();
	json_object_set_new(json_data_object, "ip", json_string(external_ip));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	free(external_ip);

	return 0;
}

void *
widget_external_ip (struct widget *widget) {
	for (;;) {
		widget_external_ip_send_update(widget);

		sleep(600);
	}

	return 0;
}
