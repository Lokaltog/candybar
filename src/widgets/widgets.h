#include <glib.h>
#include <jansson.h>
#include <string.h>
#include <webkit/webkit.h>
#include <xcb/xcb_ewmh.h>

typedef struct widget_data_t {
	char widget[32];
	char *data;
	WebKitWebView *web_view;
} widget_data_t;

typedef struct widget_t {
	void *func;
	char id[32];
	widget_data_t *data;
} widget_t;

void *widget_battery();
void *widget_desktops();
void *widget_external_ip();
void *widget_notifications();
void *widget_now_playing_mpd();
void *widget_volume();
void *widget_weather();

gboolean update_widget (widget_data_t *widget_data);
void window_object_cleared_cb(WebKitWebView  *web_view,
                              WebKitWebFrame *frame,
                              gpointer context,
                              gpointer window_object,
                              gpointer user_data);

static const widget_t widgets[] = {
#ifndef DISABLE_WIDGET_BATTERY
	(widget_t){widget_battery, "battery"},
#endif
#ifndef DISABLE_WIDGET_DESKTOPS
	(widget_t){widget_desktops, "desktops"},
#endif
#ifndef DISABLE_WIDGET_EXTERNAL_IP
	(widget_t){widget_external_ip, "external_ip"},
#endif
#ifndef DISABLE_WIDGET_NOTIFICATIONS
	(widget_t){widget_notifications, "notifications"},
#endif
#ifndef DISABLE_WIDGET_NOW_PLAYING_MPD
	(widget_t){widget_now_playing_mpd, "now_playing_mpd"},
#endif
#ifndef DISABLE_WIDGET_VOLUME
	(widget_t){widget_volume, "volume"},
#endif
#ifndef DISABLE_WIDGET_WEATHER
	(widget_t){widget_weather, "weather"},
#endif
};

#define MISSING_VALUE ""
#define LENGTH(X) (sizeof X / sizeof X[0])
