#include "util/curl.h"
#include "util/config.h"

static struct widget_config {
	const char *address;
	const char *username;
	const char *password;
	bool ssl_verify;
	int refresh_interval;
} widget_config_defaults = {
	.address = "imaps://imap.gmail.com:993/INBOX",
	.username = "",
	.password = "",
	.ssl_verify = true,
	.refresh_interval = 60,
};
