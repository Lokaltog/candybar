#include <glib.h>
#include <string.h>

#include "util/curl.h"
#include "src/config.h"

#define CURL_BUF_SIZE (256 * 1024)
#define WEATHER_BUF_SIZE 512

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
