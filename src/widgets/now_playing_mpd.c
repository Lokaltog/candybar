#include "widgets.h"
#include "now_playing_mpd.h"

static int
widget_update (struct widget *widget, struct mpd_connection *connection) {
	json_t *json_data_object = json_object();

	struct mpd_song *song;
	struct mpd_status *status;
	enum mpd_state state;

	/* get mpd status */
	mpd_send_status(connection);
	status = mpd_recv_status(connection);
	if (status == NULL) {
		LOG_ERR("status error: %s", mpd_connection_get_error_message(connection));

		return -1;
	}
	state = mpd_status_get_state(status);
	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		LOG_ERR("state error: %s", mpd_connection_get_error_message(connection));

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
				LOG_ERR("song error: %s", mpd_connection_get_error_message(connection));

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

	widget_send_update(json_data_object, widget);

	return 0;
}

void*
widget_init (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "host", config.host);
	widget_init_config_integer(widget->config, "port", config.port);
	widget_init_config_integer(widget->config, "timeout", config.timeout);

	unsigned short i;
	int mpd_fd;
	struct mpd_connection *conn = mpd_connection_new(config.host, config.port, config.timeout);
	struct epoll_event mpd_event;

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		LOG_ERR("failed to connect to mpd server at %s:%i: %s",
		        config.host, config.port,
		        mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);

		return 0;
	}

	widget_epoll_init(widget);
	mpd_fd = mpd_connection_get_fd(conn);
	mpd_event.data.fd = mpd_fd;
	mpd_event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, mpd_fd, &mpd_event) == -1) {
		LOG_ERR("failed to add fd to epoll instance: %s", strerror(errno));

		return 0;
	}

	widget_update(widget, conn);
	while (true) {
		while ((nfds = epoll_wait(efd, events, MAX_EVENTS, -1)) > 0) {
			for (i = 0; i < nfds; i++) {
				if (events[i].data.fd == widget->wkline->efd) {
					goto cleanup;
				}
				else if (events[i].data.fd == mpd_fd) {
					/* empty event buffer */
					mpd_recv_idle(conn, true);
					if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
						LOG_ERR("recv error: %s", mpd_connection_get_error_message(conn));
						break;
					}
					widget_update(widget, conn);
				}
			}
		}
	}

cleanup:
	mpd_connection_free(conn);

	return 0;
}
