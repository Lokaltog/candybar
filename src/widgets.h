#include "widgets/datetime.c"
#include "widgets/desktops.c"
#include "widgets/weather.c"

#define WIDGETS_LEN 3
static const void *wkline_enabled_widgets[WIDGETS_LEN] = {
	widget_desktops,
	widget_datetime,
	widget_weather,
};
