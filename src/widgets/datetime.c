#include "widgets.h"
#include "datetime.h"

static int
widget_send_update (struct widget *widget, struct widget_config config) {
	char *json_payload;
	time_t t;
	struct tm *tmp;
	char timestr[200];
	char datestr[200];
	json_t *json_data_object;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		perror("localtime");

		return 0;
	}

	strftime(datestr, sizeof(datestr), config.date_format, tmp);
	strftime(timestr, sizeof(timestr), config.time_format, tmp);

	json_data_object = json_object();
	json_object_set_new(json_data_object, "date", json_string(datestr));
	json_object_set_new(json_data_object, "time", json_string(timestr));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	return 0;
}

void*
widget_init (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget, "date_format", config.date_format);
	widget_init_config_string(widget, "time_format", config.time_format);

	for (;;) {
		widget_send_update(widget, config);

		sleep(1);
	}
}
