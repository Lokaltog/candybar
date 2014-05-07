static struct widget_config {
	const char *date_format;
	const char *time_format;
	int refresh_interval;
} widget_config_defaults = {
	.date_format = "%Y-%m-%d",
	.time_format = "%H:%M",
	.refresh_interval = 10,
};
