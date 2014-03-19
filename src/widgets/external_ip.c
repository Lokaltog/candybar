#include "widgets.h"
#include "external_ip.h"

void*
widget_init (struct widget *widget) {
	char *external_ip;
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "address", config.address);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	widget_epoll_init(widget);
	while (true) {
		external_ip = wkline_curl_request(config.address);
		widget_data_callback(widget,
		                     { kJSTypeString, .value.string = external_ip });
		free(external_ip);

		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:

	return 0;
}
