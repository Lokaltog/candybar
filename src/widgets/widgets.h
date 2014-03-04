#include <glib.h>
#include <jansson.h>
#include <string.h>
#include <webkit/webkit.h>
#include <xcb/xcb_ewmh.h>

struct widget {
	const char *name;
	json_t *config;
	WebKitWebView *web_view;
	char *data;
};
struct widget_call {
	void *func;
	const char *name;
};

void *widget_battery();
void *widget_desktops();
void *widget_external_ip();
void *widget_notifications();
void *widget_now_playing_mpd();
void *widget_volume();
void *widget_weather();
void *widget_window_title();
gboolean update_widget (struct widget *widget);
void window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data);

static const struct widget_call wkline_widgets[] = {
	{.func=widget_battery,         .name="battery"},
	{.func=widget_desktops,        .name="desktops"},
	{.func=widget_external_ip,     .name="external_ip"},
	{.func=widget_notifications,   .name="notifications"},
	{.func=widget_now_playing_mpd, .name="now_playing_mpd"},
	{.func=widget_volume,          .name="volume"},
	{.func=widget_weather,         .name="weather"},
	{.func=widget_window_title,    .name="window_title"},
};

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])
