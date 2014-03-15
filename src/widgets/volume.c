#include "widgets.h"
#include "volume.h"

static int
widget_update (struct widget *widget, snd_mixer_elem_t *elem) {
	long volume_min, volume_max, volume;
	int muted;

	snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);
	volume *= muted; /* if muted set volume to 0 */

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "percent", json_real(100 * (volume - volume_min) / (volume_max - volume_min)));

	widget_send_update(json_data_object, widget);

	return 0;
}

static void
widget_cleanup (void *arg) {
	LOG_DEBUG("cleanup");

	void **cleanup_data = arg;

	if (cleanup_data[0] != NULL) {
		free(cleanup_data[0]);
	}
	if (cleanup_data[1] != NULL) {
		snd_mixer_close(cleanup_data[1]);
	}
	free(arg);
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "card", config.card);
	widget_init_config_string(widget->config, "selem", config.selem);

	snd_mixer_t *mixer = NULL;
	snd_mixer_selem_id_t *sid = NULL;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err = 0, wait_err;
	unsigned short revents;

	/* open mixer */
	if ((err = snd_mixer_open(&mixer, 0)) < 0) {
		LOG_ERR("could not open mixer (%i)", err);
		goto cleanup;
	}
	if ((err = snd_mixer_attach(mixer, config.card)) < 0) {
		LOG_ERR("could not attach card '%s' to mixer (%i)", config.card, err);
		goto cleanup;
	}
	if ((err = snd_mixer_selem_register(mixer, NULL, NULL)) < 0) {
		LOG_ERR("could not register mixer simple element class (%i)", err);
		goto cleanup;
	}
	if ((err = snd_mixer_load(mixer)) < 0) {
		LOG_ERR("could not load mixer (%i)", err);
		goto cleanup;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, config.selem);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer, sid);

	void **cleanup_data = malloc(sizeof(void*) * 2);
	cleanup_data[0] = pollfds;
	cleanup_data[1] = mixer;

	pthread_cleanup_push(widget_cleanup, cleanup_data);
	widget_update(widget, elem);
	for (;;) {
		/* Code mostly from the alsamixer main loop */
		n = 1 + snd_mixer_poll_descriptors_count(mixer);
		if (n != nfds) {
			free(pollfds);
			nfds = n;
			pollfds = calloc(nfds, sizeof *pollfds);
			pollfds[0].fd = fileno(stdin);
			pollfds[0].events = POLLIN;
		}
		err = snd_mixer_poll_descriptors(mixer, &pollfds[1], nfds - 1);
		if (err < 0) {
			LOG_ERR("alsa: can't get poll descriptors: %i", err);
			break;
		}
		wait_err = snd_mixer_wait(mixer, -1);
		if (wait_err < 0) {
			LOG_ERR("alsa: wait error");
		}
		n = poll(pollfds, nfds, -1);
		if (n < 0) {
			if (errno == EINTR) {
				pollfds[0].revents = 0;
			}
			else {
				LOG_ERR("alsa: poll error");
				break;
			}
		}
		if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (n > 0) {
			err = snd_mixer_poll_descriptors_revents(mixer, &pollfds[1], nfds - 1, &revents);
			if (err < 0) {
				LOG_ERR("alsa: fatal error: %i", err);
				break;
			}
			if (revents & (POLLERR | POLLNVAL)) {
				LOG_INFO("alsa: closing mixer");
				break;
			}
			else if (revents & POLLIN) {
				snd_mixer_handle_events(mixer);
			}
		}

		widget_update(widget, elem);
	}
	pthread_cleanup_pop(1);

cleanup:
	if (pollfds != NULL) {
		free(pollfds);
	}
	if (mixer != NULL) {
		snd_mixer_close(mixer);
	}

	return 0;
}
