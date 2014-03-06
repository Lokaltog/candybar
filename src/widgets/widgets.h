#include <jansson.h>
#include <string.h>
#include <webkit/webkit.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

void* widget_battery ();
void* widget_desktops ();
void* widget_external_ip ();
void* widget_notifications ();
void* widget_now_playing_mpd ();
void* widget_volume ();
void* widget_weather ();
void* widget_window_title ();
gboolean update_widget (struct widget *widget);
void window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data);

static const struct widget_call wkline_widgets[] = {
#ifndef DISABLE_WIDGET_BATTERY
	{ .func = widget_battery, .name = "battery" },
#endif
#ifndef DISABLE_WIDGET_DESKTOPS
	{ .func = widget_desktops, .name = "desktops" },
#endif
#ifndef DISABLE_WIDGET_EXTERNAL_IP
	{ .func = widget_external_ip, .name = "external_ip" },
#endif
#ifndef DISABLE_WIDGET_NOTIFICATIONS
	{ .func = widget_notifications, .name = "notifications" },
#endif
#ifndef DISABLE_WIDGET_NOW_PLAYING_MPD
	{ .func = widget_now_playing_mpd, .name = "now_playing_mpd" },
#endif
#ifndef DISABLE_WIDGET_VOLUME
	{ .func = widget_volume, .name = "volume" },
#endif
#ifndef DISABLE_WIDGET_WEATHER
	{ .func = widget_weather, .name = "weather" },
#endif
#ifndef DISABLE_WIDGET_WINDOW_TITLE
	{ .func = widget_window_title, .name = "window_title" },
#endif
};

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])
