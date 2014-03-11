#include <pthread.h>

#include "util/curl.h"
#include "util/config.h"

static struct widget_config {
	const char *address;
	int refresh_interval;
} widget_config_defaults = {
	.address = "http://ipv4.icanhazip.com/",
	.refresh_interval = 3600,
};
