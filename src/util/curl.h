#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "util/log.h"

#define CURL_BUF_SIZE (256 * 1024)

typedef struct write_result_t {
	char *data;
	int pos;
} write_result_t;

size_t candybar_curl_write_response (void *ptr, size_t size, size_t nmemb, void *stream);
char* candybar_curl_request (const char *url);
