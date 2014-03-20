#include "widgets.h"
#include "volume.h"

static bool
mixer_init (struct widget *widget, snd_mixer_t **mixer, snd_mixer_elem_t **selem) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "card", config.card);
	widget_init_config_string(widget->config, "selem", config.selem);

	/* open mixer */
	int err = 0;
	snd_mixer_selem_id_t *sid = NULL;

	if ((err = snd_mixer_open(mixer, 0)) < 0) {
		LOG_ERR("could not open mixer (%i)", err);

		return false;
	}
	if ((err = snd_mixer_attach(*mixer, config.card)) < 0) {
		LOG_ERR("could not attach card '%s' to mixer (%i)", config.card, err);

		return false;
	}
	if ((err = snd_mixer_selem_register(*mixer, NULL, NULL)) < 0) {
		LOG_ERR("could not register mixer simple element class (%i)", err);

		return false;
	}
	if ((err = snd_mixer_load(*mixer)) < 0) {
		LOG_ERR("could not load mixer (%i)", err);

		return false;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, config.selem);

	*selem = snd_mixer_find_selem(*mixer, sid);
	if (*selem == NULL) {
		LOG_ERR("could not find selem '%s'", config.selem);

		return false;
	}

	return true;
}

static JSValueRef
widget_js_func_set_active (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	if (!argc) {
		LOG_WARN("set_active: requires at least one argument");
		goto cleanup;
	}
	if (!JSValueIsBoolean(ctx, argv[0])) {
		LOG_WARN("set_active: argument 1 must be a boolean");
		goto cleanup;
	}

	struct widget *widget = JSObjectGetPrivate(this);
	bool active = JSValueToBoolean(ctx, argv[0]);

	snd_mixer_t *mixer = NULL;
	snd_mixer_elem_t *selem = NULL;
	if (!mixer_init(widget, &mixer, &selem)) {
		goto cleanup;
	}

	snd_mixer_selem_set_playback_switch_all(selem, active);

cleanup:
	if (mixer != NULL) {
		snd_mixer_close(mixer);
	}

	return JSValueMakeUndefined(ctx);
}

const JSStaticFunction widget_js_staticfuncs[] = {
	{ "set_active", widget_js_func_set_active, kJSPropertyAttributeReadOnly },
	{ NULL, NULL, 0 },
};

static int
widget_update (struct widget *widget, snd_mixer_elem_t *elem) {
	long volume_min, volume_max, volume;
	int active;

	snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &active);

	widget_data_callback(widget,
	                     widget_data_arg_number(100 * (volume - volume_min) / (volume_max - volume_min)),
	                     widget_data_arg_boolean(active));

	return 0;
}

void*
widget_init (struct widget *widget) {
	/* open mixer */
	int err = 0;
	int mixer_fd;
	struct epoll_event mixer_event;
	struct pollfd *pollfds = NULL;
	unsigned short i;

	snd_mixer_t *mixer = NULL;
	snd_mixer_elem_t *selem = NULL;
	if (!mixer_init(widget, &mixer, &selem)) {
		goto cleanup;
	}

	widget_epoll_init(widget);

	int pollfds_len = snd_mixer_poll_descriptors_count(mixer);
	pollfds = calloc(pollfds_len, sizeof(*pollfds));
	err = snd_mixer_poll_descriptors(mixer, &pollfds[0], pollfds_len);
	if (err < 0) {
		LOG_ERR("alsa: can't get poll descriptors: %i", err);
		goto cleanup;
	}

	for (i = 0; i < pollfds_len; i++) {
		mixer_fd = pollfds[i].fd;
		mixer_event.data.fd = mixer_fd;
		mixer_event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, mixer_fd, &mixer_event) == -1) {
			LOG_ERR("failed to add fd to epoll instance: %s", strerror(errno));
			goto cleanup;
		}
	}

	widget_update(widget, selem);
	while (true) {
		while ((nfds = epoll_wait(efd, events, MAX_EVENTS, -1)) > 0) {
			for (i = 0; i < nfds; i++) {
				if (events[i].data.fd == widget->wkline->efd) {
					goto cleanup;
				}
			}
			snd_mixer_handle_events(mixer);
			widget_update(widget, selem);
		}
	}

cleanup:
	if (pollfds != NULL) {
		free(pollfds);
	}
	if (mixer != NULL) {
		snd_mixer_close(mixer);
	}

	return 0;
}
