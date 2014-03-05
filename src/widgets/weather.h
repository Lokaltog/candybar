#include <glib.h>
#include <string.h>

#include "util/curl.h"
#include "util/load_config.h"

#define WEATHER_BUF_SIZE 512

struct location_t {
	char *city;
	char *region_code;
	char *country_code;
	char *unit;
};

struct weather_t {
	int code;
	double temp;
	char *unit;
};
