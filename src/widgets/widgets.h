#include <glib.h>
#include <jansson.h>
#include <xcb/xcb_ewmh.h>

typedef struct widget_data_t {
	char *widget;
	char *data;
} widget_data_t;

struct wkline_widget_t{
	char *name;
	json_t *config;
};

void *widget_battery();
void *widget_desktops();
void *widget_external_ip();
void *widget_notifications();
void *widget_now_playing_mpd();
void *widget_volume();
void *widget_weather();

gboolean update_widget (widget_data_t *widget_data);

struct widget_call {
	void *call;
	const char *name;
};

static const struct widget_call wkline_widgets[] = {
	{.call = widget_battery, .name = "battery"},
	{.call = widget_desktops, .name = "desktops"},
	{.call = widget_external_ip, .name = "external_ip"},
	{.call = widget_notifications, .name = "notifications"},
	{.call = widget_now_playing_mpd, .name = "now_playing_mpd"},
	{.call = widget_volume, .name = "volume"},
	{.call = widget_weather, .name = "weather"}
};

#define MISSING_VALUE ""
