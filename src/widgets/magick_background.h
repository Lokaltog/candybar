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
	int contrast;
	int height;
	int saturation;
} widget_config_defaults = {
	.image = "",
	.css_gradient_overlay_start = "transparent",
	.css_gradient_overlay_end = "rgba(0, 0, 0, .5)",
	.blur_radius = 0,
	.brightness = 0,
	.contrast = 0,
	.height = 40,
	.saturation = 100,
};
