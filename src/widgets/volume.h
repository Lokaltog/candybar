#include <alsa/asoundlib.h>
#include <pthread.h>
#include <webkit/webkit.h>

#include "util/wkconfig.h"

struct widget_volume_res {
	struct pollfd *pollfds;
	snd_mixer_t *mixer;
};
