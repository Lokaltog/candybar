#include <mpd/client.h>

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
