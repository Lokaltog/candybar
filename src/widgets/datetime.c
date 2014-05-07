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

	widget_data_callback(widget,
	                     widget_data_arg_string(datestr),
	                     widget_data_arg_string(timestr));

	return 0;
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "date_format", config.date_format);
	widget_init_config_string(widget->config, "time_format", config.time_format);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	widget_epoll_init(widget);
	while (true) {
		widget_update(widget, config);
		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:

	widget_epoll_cleanup(widget);
	pthread_exit(0);
}
