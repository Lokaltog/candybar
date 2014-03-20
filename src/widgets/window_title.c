#include "widgets.h"
#include "window_title.h"

static int
widget_update (struct widget *widget, xcb_ewmh_connection_t *ewmh, int screen_nbr, xcb_window_t *cur_win) {
	xcb_window_t win;
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	xcb_icccm_get_text_property_reply_t icccm_txt_prop;
	uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
	uint32_t values_reset[] = { XCB_EVENT_MASK_NO_EVENT };
	char window_title[BUFSIZ];

	ewmh_txt_prop.strings = NULL;
	icccm_txt_prop.name = NULL;

	if ((xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, screen_nbr), &win, NULL) == 1)
	    && ((xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, win), &ewmh_txt_prop, NULL) == 1)
	        || (xcb_icccm_get_wm_name_reply(ewmh->connection, xcb_icccm_get_wm_name(ewmh->connection, win), &icccm_txt_prop, NULL) == 1))) {
		if ((ewmh_txt_prop.strings != NULL) && (ewmh_txt_prop.strings_len > 0)) {
			copy_prop(window_title, ewmh_txt_prop.strings, ewmh_txt_prop.strings_len, 0, 1);
		}
		else if ((icccm_txt_prop.name != NULL) && (icccm_txt_prop.name_len > 0)) {
			copy_prop(window_title, icccm_txt_prop.name, icccm_txt_prop.name_len, 0, 1);
		}
		else {
			strcpy(window_title, MISSING_VALUE);
		}
		if (win != *cur_win) {
			xcb_change_window_attributes(ewmh->connection, *cur_win, XCB_CW_EVENT_MASK, values_reset);
			cur_win = &win;
		}
		xcb_generic_error_t *err = xcb_request_check(ewmh->connection, xcb_change_window_attributes_checked(ewmh->connection, win, XCB_CW_EVENT_MASK, values));
		if (err != NULL) {
			LOG_INFO("could not capture property change events on window 0x%X", win);
		}
	}
	else {
		strcpy(window_title, MISSING_VALUE);
	}

	widget_data_callback(widget, widget_data_arg_string(window_title));

	return 0;
}

void*
widget_init (struct widget *widget) {
	unsigned short i;
	int xcb_fd;
	int screen_nbr = 0;
	xcb_window_t cur_win = 0;
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	struct epoll_event xcb_event;

	if (xcb_connection_has_error(conn)) {
		LOG_ERR("could not connect to display %s", getenv("DISPLAY"));
		goto cleanup;
	}

	xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

	uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
	xcb_generic_event_t *evt;
	xcb_generic_error_t *err = xcb_request_check(ewmh->connection,
	                                             xcb_change_window_attributes_checked(ewmh->connection,
	                                                                                  ewmh->screens[screen_nbr]->root,
	                                                                                  XCB_CW_EVENT_MASK,
	                                                                                  values));

	if (err != NULL) {
		LOG_ERR("could not request EWMH property change notifications");
		goto cleanup;
	}

	widget_epoll_init(widget);
	xcb_fd = xcb_get_file_descriptor(ewmh->connection);
	xcb_event.data.fd = xcb_fd;
	xcb_event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, xcb_fd, &xcb_event) == -1) {
		LOG_ERR("failed to add fd to epoll instance: %s", strerror(errno));

		return 0;
	}

	widget_update(widget, ewmh, screen_nbr, &cur_win);
	while (true) {
		while ((nfds = epoll_wait(efd, events, MAX_EVENTS, -1)) > 0) {
			for (i = 0; i < nfds; i++) {
				if (events[i].data.fd == widget->wkline->efd) {
					goto cleanup;
				}
			}

			while ((evt = xcb_poll_for_event(ewmh->connection)) != NULL) {
				xcb_property_notify_event_t *pne;
				switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
				case XCB_PROPERTY_NOTIFY:
					pne = (xcb_property_notify_event_t*)evt;
					if (pne->atom == ewmh->_NET_ACTIVE_WINDOW) {
						widget_update(widget, ewmh, screen_nbr, &cur_win);
					}
					else if ((pne->window != ewmh->screens[screen_nbr]->root) && ((pne->atom == ewmh->_NET_WM_NAME) || (pne->atom == XCB_ATOM_WM_NAME))) {
						widget_update(widget, ewmh, screen_nbr, &cur_win);
					}
				default:
					break;
				}
				free(evt);
			}
		}
	}

cleanup:
	if (ewmh != NULL) {
		xcb_ewmh_connection_wipe(ewmh);
	}
	if (conn != NULL) {
		xcb_disconnect(conn);
	}

	return 0;
}
