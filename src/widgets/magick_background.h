#include <pthread.h>
#include <string.h>
#include <wand/magick_wand.h>

#include "util/config.h"

static struct widget_config {
	const char *image;
	const char *css_gradient_overlay_start;
	const char *css_gradient_overlay_end;
	int blur_radius;
	int brightness;
	int height;
	int saturation;
} widget_config_defaults = {
	.image = "",
	.css_gradient_overlay_start = "transparent",
	.css_gradient_overlay_end = "rgba(0, 0, 0, .4)",
	.blur_radius = 0,
	.brightness = 100,
	.height = 40,
	.saturation = 100,
};
