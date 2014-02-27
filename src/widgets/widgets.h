#include <glib.h>
#include <jansson.h>
#include <xcb/xcb_ewmh.h>

typedef struct thread_data_t {
	int screen_nbr;
	xcb_ewmh_connection_t *ewmh;
} thread_data_t;

typedef struct widget_data_t {
	char *widget;
	char *data;
} widget_data_t;

extern thread_data_t thread_data;

void *widget_weather();
void *widget_volume();
void *widget_now_playing_mpd();
void *widget_desktops();

gboolean update_widget (widget_data_t *widget_data);

#define WIDGETS_LEN 4
static const void *wkline_widgets[WIDGETS_LEN] = {
#ifndef DISABLE_DESKTOPS
	widget_desktops,
#endif
#ifndef DISABLE_NOW_PLAYING
	widget_now_playing_mpd,
#endif
#ifndef DISABLE_VOLUME
	widget_volume,
#endif
#ifndef DISABLE_WEATHER
	widget_weather,
#endif
};
