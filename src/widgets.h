#ifndef DISABLE_DATETIME
#include "widgets/datetime.c"
#endif
#ifndef DISABLE_DESKTOPS
#include "widgets/desktops.c"
#endif
#ifndef DISABLE_NOW_PLAYING
#include "widgets/now_playing_mpd.c"
#endif
#ifndef DISABLE_VOLUME
#include "widgets/volume.c"
#endif
#ifndef DISABLE_WEATHER
#include "widgets/weather.c"
#endif

#define WIDGETS_LEN 5
static const void *wkline_widgets[WIDGETS_LEN] = {
#ifndef DISABLE_DATETIME
	widget_datetime,
#endif
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
