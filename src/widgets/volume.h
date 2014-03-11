#include <alsa/asoundlib.h>
#include <pthread.h>
#include <webkit/webkit.h>

#include "util/config.h"

struct widget_volume_res {
	struct pollfd *pollfds;
	snd_mixer_t *mixer;
};

static struct widget_config {
	const char *card;
	const char *selem;
} widget_config_defaults = {
	.card = "default",
	.selem = "Master",
};
