#include <wand/magick_wand.h>

static struct widget_config {
	const char *image;
	const char *css_gradient_overlay;
	int blur_radius;
	int brightness;
	int height;
	int saturation;
} widget_config_defaults = {
	.image = "",
	.css_gradient_overlay = "top, transparent, rgba(0, 0, 0, .5)",
	.blur_radius = 0,
	.brightness = 100,
	.height = 40,
	.saturation = 100,
};
