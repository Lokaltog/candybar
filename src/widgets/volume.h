#include <alsa/asoundlib.h>

static struct widget_config {
	const char *card;
	const char *selem;
} widget_config_defaults = {
	.card = "default",
	.selem = "Master",
};
