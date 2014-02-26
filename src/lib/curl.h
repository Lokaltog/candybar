#include <curl/curl.h>

#define CURL_BUF_SIZE (256 * 1024)
#define WEATHER_BUF_SIZE 512

typedef struct write_result_t {
    char *data;
    int pos;
} write_result_t;

static size_t wkline_curl_write_response (void *ptr, size_t size, size_t nmemb, void *stream);
static char *wkline_curl_request (const char *url);

#include "curl.c"
