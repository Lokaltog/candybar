void
*widget_volume (thread_data_t *thread_data) {
	json_t *json_base_object;
	json_t *json_data_object;
	char *json_payload;

	long volume_min, volume_max, volume = -1;
	snd_mixer_t *mixer;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err;
	unsigned short revents;
	bool first_iter = true;

	// open mixer
	snd_mixer_open(&mixer, 0);
	snd_mixer_attach(mixer, card);
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(mixer, sid);

	for (;;) {
		if (first_iter) {
			// FIXME should probably find a better way to do this
			first_iter = false;
			goto update_volume;
		}

		// TODO check if channel is muted
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
			fprintf(stderr, "widget_volume: can't get poll descriptors: %i\n", err);
			break;
		}
		n = poll(pollfds, nfds, -1);
		if (n < 0) {
			if (errno == EINTR) {
				pollfds[0].revents = 0;
			}
			else {
				fprintf(stderr, "widget_volume: poll error\n");
				break;
			}
		}
		if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (n > 0) {
			err = snd_mixer_poll_descriptors_revents(mixer, &pollfds[1], nfds - 1, &revents);
			if (err < 0) {
				fprintf(stderr, "widget_volume: fatal error: %i\n", err);
				break;
			}
			if (revents & (POLLERR | POLLNVAL)){
				fprintf(stderr, "widget_volume: close mixer\n");
				break;
			}
			else if (revents & POLLIN) {
				snd_mixer_handle_events(mixer);
			}
		}

	update_volume:
		snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);

		volume -= volume_min;
		volume_max -= volume_min;
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);

		json_base_object = json_object();
		json_data_object = json_object();
		json_object_set_new(json_base_object, "widget", json_string("volume"));
		json_object_set_new(json_base_object, "data", json_data_object);

		json_object_set_new(json_data_object, "volume_percent", json_integer(100 * volume / volume_max));

		json_payload = json_dumps(json_base_object, 0);

		// inject data
		g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);
	}

	free(pollfds);
	snd_mixer_close(mixer);
	return 0;
}
