#include <curl/curl.h>

#define CURL_BUF_SIZE (256 * 1024)
#define WEATHER_BUF_SIZE 512

typedef struct write_result_t {
    char *data;
    int pos;
} write_result_t;

typedef struct location_t {
	char *city;
	char *region_code;
	char *country_code;
} location_t;

typedef struct weather_t {
	int code;
	double temp;
	char unit[1];
} weather_t;

static size_t write_response (void *ptr, size_t size, size_t nmemb, void *stream);
static char *request (const char *url);
static location_t *get_geoip_location ();
static weather_t *get_weather_information (location_t *location, const char *unit);
