#include <glib.h>
#include <jansson.h>
#include <xcb/xcb_ewmh.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>

typedef struct widget_data_t {
	char *widget;
	char *data;
} widget_data_t;

void *widget_battery();
void *widget_desktops();
void *widget_notifications();
void *widget_now_playing_mpd();
void *widget_weather();

gboolean update_widget (widget_data_t *widget_data);

static const void *wkline_widgets[] = {
#ifndef DISABLE_WIDGET_BATTERY
	widget_battery,
#endif
#ifndef DISABLE_WIDGET_DESKTOPS
	widget_desktops,
#endif
#ifndef DISABLE_WIDGET_NOTIFICATIONS
	widget_notifications,
#endif
#ifndef DISABLE_WIDGET_NOW_PLAYING_MPD
	widget_now_playing_mpd,
#endif
#ifndef DISABLE_WIDGET_WEATHER
	widget_weather,
#endif
};

#define MISSING_VALUE ""

void
window_object_cleared_cb(WebKitWebView  *web_view,
							WebKitWebFrame *frame,
							gpointer context,
							gpointer window_object,
							gpointer user_data);