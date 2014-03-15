#include "util/curl.h"

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

static struct widget_config {
	const char *location;
	const char *unit;
	int refresh_interval;
} widget_config_defaults = {
	.location = "",
	.unit = "c",
	.refresh_interval = 1800,
};
