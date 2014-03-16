#include "widgets.h"
#include "volume.h"

static ref_ctx_t *ref_ctx = NULL;

static void
widget_js_init_cb (JSContextRef ctx, JSObjectRef object) {
	ref_ctx = calloc(1, sizeof(ref_ctx_t));
	ref_ctx->context = ctx;
	ref_ctx->object = object;
}

static const JSClassDefinition widget_js_def = {
	0,
	kJSClassAttributeNone,
	"VolumeWidget",
	NULL, NULL, NULL,
	widget_js_init_cb,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static int
widget_update (struct widget *widget, snd_mixer_elem_t *elem) {
	long volume_min, volume_max, volume;
	int muted;

	snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);

	/* call onDataChanged callback */
	JSStringRef string_ondatachanged = JSStringCreateWithUTF8CString("onDataChanged");
	JSValueRef func = JSObjectGetProperty(ref_ctx->context, ref_ctx->object, string_ondatachanged, NULL);
	JSObjectRef function = JSValueToObject(ref_ctx->context, func, NULL);
	JSValueRef function_args[] = {
		JSValueMakeNumber(ref_ctx->context, 100 * (volume - volume_min) / (volume_max - volume_min)),
		JSValueMakeBoolean(ref_ctx->context, !muted),
	};
	JSStringRelease(string_ondatachanged);

	if (!JSObjectIsFunction(ref_ctx->context, function)) {
		LOG_DEBUG("onDataChanged is not function or is not set");

		return 1;
	}

	JSObjectCallAsFunction(ref_ctx->context, function, ref_ctx->object, LENGTH(function_args), function_args, NULL);

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
	if (ref_ctx != NULL) {
		free(ref_ctx);
	}
	free(arg);
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "card", config.card);
	widget_init_config_string(widget->config, "selem", config.selem);

	/* init js object */
	JSClassRef class_def = JSClassCreate(&widget_js_def);
	JSObjectRef class_obj = JSObjectMake(widget->wkline->wk_context, class_def, widget->wkline->wk_context);
	JSObjectRef global_obj = JSContextGetGlobalObject(widget->wkline->wk_context);
	JSStringRef str = JSStringCreateWithUTF8CString("VolumeWidget");
	JSObjectSetProperty(widget->wkline->wk_context, global_obj, str, class_obj, kJSPropertyAttributeNone, NULL);

	/* open mixer */
	snd_mixer_t *mixer = NULL;
	snd_mixer_selem_id_t *sid = NULL;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err = 0, wait_err;
	unsigned short revents;

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
