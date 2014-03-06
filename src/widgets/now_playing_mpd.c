#include "widgets.h"
#include "now_playing_mpd.h"

static int
widget_now_playing_mpd_send_update (struct widget *widget, struct mpd_connection *connection) {
	json_t *json_data_object = json_object();
	char *json_payload;

	struct mpd_song *song;
	struct mpd_status *status;
	enum mpd_state state;

	/* get mpd status */
	mpd_send_status(connection);
	status = mpd_recv_status(connection);
	if (status == NULL) {
		wklog("mpd: status error: %s", mpd_connection_get_error_message(connection));

		return -1;
	}
	state = mpd_status_get_state(status);
	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		wklog("mpd: state error: %s", mpd_connection_get_error_message(connection));

		return -1;
	}

	/* only update if playing/paused */
	if (!((state == MPD_STATE_STOP) || (state == MPD_STATE_UNKNOWN))) {
		mpd_send_current_song(connection);

		while ((song = mpd_recv_song(connection)) != NULL) {
			json_object_set_new(json_data_object, "title", json_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0)));
			json_object_set_new(json_data_object, "artist", json_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)));
			json_object_set_new(json_data_object, "album", json_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)));
			json_object_set_new(json_data_object, "total_sec", json_integer(mpd_status_get_total_time(status)));
			json_object_set_new(json_data_object, "elapsed_sec", json_integer(mpd_status_get_elapsed_time(status)));
			json_object_set_new(json_data_object, "playing", json_boolean(state == MPD_STATE_PLAY));

			mpd_song_free(song);

			if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
				wklog("mpd: song error: %s", mpd_connection_get_error_message(connection));

				return -1;
			}
		}
	}
	else {
		json_object_set_new(json_data_object, "title", json_null());
		json_object_set_new(json_data_object, "artist", json_null());
		json_object_set_new(json_data_object, "album", json_null());
		json_object_set_new(json_data_object, "total_sec", json_null());
		json_object_set_new(json_data_object, "elapsed_sec", json_null());
		json_object_set_new(json_data_object, "playing", json_null());
	}

	mpd_status_free(status);
	mpd_send_idle_mask(connection, MPD_IDLE_PLAYER);

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);

	return 0;
}

void*
widget_now_playing_mpd (struct widget *widget) {
	struct mpd_connection *connection = mpd_connection_new(json_string_value(wkline_widget_get_config(widget, "host")),
	                                                       json_integer_value(wkline_widget_get_config(widget, "port")),
	                                                       json_integer_value(wkline_widget_get_config(widget, "timeout")));

	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		wklog("mpd: failed to connect to mpd server at %s:%i: %s",
		      json_string_value(wkline_widget_get_config(widget, "host")),
		      json_integer_value(wkline_widget_get_config(widget, "port")),
		      mpd_connection_get_error_message(connection));
		mpd_connection_free(connection);

		return 0;
	}

	fd_set fds;
	int s, mpd_fd = mpd_connection_get_fd(connection);

	widget_now_playing_mpd_send_update(widget, connection);

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(mpd_fd, &fds);

		s = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
		if (s < 0) {
			wklog("mpd: select error");
			break;
		}
		if (!s) {
			wklog("mpd: select timeout");
			break;
		}

		if (FD_ISSET(mpd_fd, &fds)) {
			/* empty event buffer */
			mpd_recv_idle(connection, true);
			if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
				wklog("mpd: recv error: %s", mpd_connection_get_error_message(connection));
				break;
			}
			widget_now_playing_mpd_send_update(widget, connection);
		}
	}

	mpd_connection_free(connection);

	return 0;
}
