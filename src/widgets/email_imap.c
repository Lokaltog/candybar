#include "widgets.h"
#include "email_imap.h"

static int
widget_update (struct widget *widget, struct widget_config config) {
	CURL *curl;
	CURLcode status;
	char *data;
	long code;

	/* connect and handle IMAP responses */
	curl = curl_easy_init();
	data = malloc(CURL_BUF_SIZE);
	if (!curl || !data) {
		return 0;
	}

	write_result_t write_result = {
		.data = data,
		.pos = 0
	};

	curl_easy_setopt(curl, CURLOPT_USERNAME, config.username);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password);
	curl_easy_setopt(curl, CURLOPT_URL, config.address);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "SEARCH UNSEEN");

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wkline_curl_write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	if (config.ssl_verify) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	status = curl_easy_perform(curl);
	if (status != CURLE_OK) {
		LOG_ERR("unable to request data from %s (this error may be temporary): %s",
		        config.address,
		        curl_easy_strerror(status));

		return 0;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

	if (code != 0) {
		LOG_ERR("server responded with code %ld", code);

		return 0;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	data[write_result.pos] = '\0';

	/* count unread message IDs */

	/* the server responds with a string like "* SEARCH 1 2 3 4" where the
	   numbers are message IDs */
	char *str, *saveptr, *delim = " *";
	int unread = -1;

	str = strtok_r(data, delim, &saveptr);
	while (str != NULL) {
		str = strtok_r(NULL, delim, &saveptr);
		unread++;
	}

	free(data);

	widget_data_callback(widget,
	                     widget_data_arg_number(unread),
	                     widget_data_arg_string(config.username));

	return 0;
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "address", config.address);
	widget_init_config_string(widget->config, "username", config.username);
	widget_init_config_string(widget->config, "password", config.password);
	widget_init_config_boolean(widget->config, "ssl_verify", config.ssl_verify);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	if (!config.username) {
		LOG_INFO("email_imap: username not set, disabling widget");

		return 0;
	}

	widget_epoll_init(widget);
	while (true) {
		widget_update(widget, config);
		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:

	return 0;
}
