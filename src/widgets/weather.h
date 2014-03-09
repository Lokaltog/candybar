#include <pthread.h>

#include "util/curl.h"
#include "util/wkconfig.h"

#define WEATHER_BUF_SIZE 512

struct location {
	char *city;
	char *region_code;
	char *country_code;
};

struct weather {
	int code;
	double temp;
	char *unit;
};
