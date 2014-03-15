#include <mpd/client.h>
#include <pthread.h>
#include <sys/select.h>

#include "util/config.h"

static struct widget_config {
	const char *host;
	int port;
	int timeout;
	int update_interval;
} widget_config_defaults = {
	.host = "localhost",
	.port = 6600,
	.timeout = 5000,
	.update_interval = 1000,
};
