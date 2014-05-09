#include <mpd/client.h>

static struct widget_config {
	const char *host;
	int port;
	int timeout;
} widget_config_defaults = {
	.host = "localhost",
	.port = 6600,
	.timeout = 5000,
};
