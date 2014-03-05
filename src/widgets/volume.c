#include "widgets.h"
#include "volume.h"

static int
widget_volume_send_update (snd_mixer_elem_t* elem) {
	json_t *json_data_object = json_object();
	char *json_payload;
	long volume_min, volume_max, volume;
	int muted;

	snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT , &muted);
	volume *= muted; /* if muted set volume to 0 */

	json_object_set_new(json_data_object, "percent", json_real(100 * (volume - volume_min) / (volume_max - volume_min)));

	json_payload = json_dumps(json_data_object, 0);

	widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
	widget_data->widget = "volume";
	widget_data->data = json_payload;
	g_idle_add((GSourceFunc)update_widget, widget_data);

	return 0;
}

void *
widget_volume (struct wkline_widget_t *widget) {
	snd_mixer_t *mixer;
	snd_mixer_selem_id_t *sid;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err, wait_err;
	unsigned short revents;

	// open mixer
	snd_mixer_open(&mixer, 0);
	snd_mixer_attach(mixer, json_string_value(wkline_widget_get_config(widget, "card")));
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, json_string_value(wkline_widget_get_config(widget, "selem")));
	snd_mixer_elem_t* elem = snd_mixer_find_selem(mixer, sid);

	widget_volume_send_update(elem);

	for (;;) {
		// Code mostly from the alsamixer main loop
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
			wklog("alsa: can't get poll descriptors: %i", err);
			break;
		}
		wait_err = snd_mixer_wait(mixer, -1);
		if (wait_err < 0) {
			wklog("alsa: wait error");
		}
		n = poll(pollfds, nfds, -1);
		if (n < 0) {
			if (errno == EINTR) {
				pollfds[0].revents = 0;
			}
			else {
				wklog("alsa: poll error");
				break;
			}
		}
		if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (n > 0) {
			err = snd_mixer_poll_descriptors_revents(mixer, &pollfds[1], nfds - 1, &revents);
			if (err < 0) {
				wklog("alsa: fatal error: %i", err);
				break;
			}
			if (revents & (POLLERR | POLLNVAL)){
				wklog("alsa: closing mixer");
				break;
			}
			else if (revents & POLLIN) {
				snd_mixer_handle_events(mixer);
			}
		}

		widget_volume_send_update(elem);
	}

	free(pollfds);
	snd_mixer_close(mixer);
	return 0;
}
