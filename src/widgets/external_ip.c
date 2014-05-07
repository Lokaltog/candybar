#include "widgets.h"
#include "external_ip.h"

void*
widget_main (struct widget *widget) {
	char *external_ip;
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "address", config.address);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	widget_epoll_init(widget);
	while (true) {
		external_ip = candybar_curl_request(config.address);
		widget_data_callback(widget, widget_data_arg_string(external_ip));
		free(external_ip);

		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:

	widget_epoll_cleanup(widget);
	widget_clean_exit(widget);
}
