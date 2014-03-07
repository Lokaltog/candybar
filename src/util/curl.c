#include "curl.h"

size_t
wkline_curl_write_response (void *ptr, size_t size, size_t nmemb, void *stream) {
	write_result_t *result = (write_result_t*)stream;

	if (result->pos + size * nmemb >= CURL_BUF_SIZE - 1) {
		wklog("curl: too small buffer");

		return 0;
	}

	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;

	return size * nmemb;
}

char*
wkline_curl_request (const char *url) {
	CURL *curl;
	CURLcode status;
	char *data;
	long code;

	curl = curl_easy_init();
	data = malloc(CURL_BUF_SIZE);
	if (!curl || !data) {
		return NULL;
	}

	write_result_t write_result = {
		.data = data,
		.pos = 0
	};

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wkline_curl_write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	status = curl_easy_perform(curl);
	if (status != 0) {
		wklog("curl: unable to request data from %s:\n%s", url, curl_easy_strerror(status));

		return NULL;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		wklog("curl: server responded with code %ld", code);

		return NULL;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	data[write_result.pos] = '\0';

	return data;
}
