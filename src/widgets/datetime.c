#include "widgets.h"
#include "datetime.h"

static int
widget_update (struct widget *widget, struct widget_config config) {
	time_t t;
	struct tm *tmp;
	char timestr[200];
	char datestr[200];

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		perror("localtime");

		return 0;
	}

	strftime(datestr, sizeof(datestr), config.date_format, tmp);
	strftime(timestr, sizeof(timestr), config.time_format, tmp);

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "date", json_string(datestr));
	json_object_set_new(json_data_object, "time", json_string(timestr));

	widget_send_update(json_data_object, widget);

	return 0;
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget, "date_format", config.date_format);
	widget_init_config_string(widget, "time_format", config.time_format);
	widget_init_config_integer(widget, "refresh_interval", config.refresh_interval);

	for (;;) {
		widget_update(widget, config);

		sleep(config.refresh_interval);
	}
}
