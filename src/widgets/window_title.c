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

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "window_title", json_string(window_title));

	widget_send_update(json_data_object, widget);

	return 0;
}

static void
widget_cleanup (void *arg) {
	LOG_DEBUG("cleanup");

	void **cleanup_data = arg;

	if (cleanup_data[0] != NULL) {
		xcb_ewmh_connection_wipe(cleanup_data[0]);
	}
	if (cleanup_data[1] != NULL) {
		xcb_disconnect(cleanup_data[1]);
	}
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	int screen_nbr = widget->wkline->screen;
	xcb_window_t cur_win = 0;
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));

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

	void *cleanup_data[] = { ewmh, conn };
	pthread_cleanup_push(widget_cleanup, &cleanup_data);
	widget_update(widget, ewmh, screen_nbr, &cur_win);
	for (;;) {
		while ((evt = xcb_wait_for_event(ewmh->connection)) != NULL) {
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
	pthread_cleanup_pop(1);

cleanup:
	if (ewmh != NULL) {
		xcb_ewmh_connection_wipe(ewmh);
	}
	if (conn != NULL) {
		xcb_disconnect(conn);
	}

	return 0;
}
