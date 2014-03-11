#include "widgets.h"
#include "email_imap.h"

static int
send_update (struct widget *widget, struct widget_config config) {
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
		LOG_ERR("curl/email_imap: unable to request data from %s (this error may be temporary): %s",
		        config.address,
		        curl_easy_strerror(status));

		return 0;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

	if (code != 0) {
		LOG_ERR("curl/email_imap: server responded with code %ld", code);

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

	/* send json data */
	char *json_payload;
	json_t *json_data_object;

	json_data_object = json_object();
	json_object_set_new(json_data_object, "username", json_string(config.username));
	json_object_set_new(json_data_object, "unread", json_integer(unread));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	return 0;
}

void*
widget_init (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget, "address", config.address);
	widget_init_config_string(widget, "username", config.username);
	widget_init_config_string(widget, "password", config.password);
	widget_init_config_boolean(widget, "ssl_verify", config.ssl_verify);

	if (!config.username) {
		LOG_INFO("email_imap: username not set, disabling widget");

		return 0;
	}

	for (;;) {
		send_update(widget, config);

		sleep(60);
	}
}
