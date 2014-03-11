#include "util/config.h"

static struct widget_config {
	const char *date_format;
	const char *time_format;
} widget_config_defaults = {
	.date_format = "%Y-%m-%d",
	.time_format = "%H:%M:%S",
};
