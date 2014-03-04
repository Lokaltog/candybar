#include "widgets.h"
#include "window_title.h"

static void
window_title_send_update (struct widget *widget, xcb_ewmh_connection_t *ewmh, int screen_nbr, xcb_window_t *cur_win) {
	xcb_window_t win;
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	xcb_icccm_get_text_property_reply_t icccm_txt_prop;
	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	uint32_t values_reset[] = {XCB_EVENT_MASK_NO_EVENT};
	char window_title[BUFSIZ];

	ewmh_txt_prop.strings = NULL;
	icccm_txt_prop.name = NULL;

	if (xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, screen_nbr), &win, NULL) == 1
	    && (xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, win), &ewmh_txt_prop, NULL) == 1
	        || xcb_icccm_get_wm_name_reply(ewmh->connection, xcb_icccm_get_wm_name(ewmh->connection, win), &icccm_txt_prop, NULL) == 1)) {
		if (ewmh_txt_prop.strings != NULL && ewmh_txt_prop.strings_len > 0) {
			copy_prop(window_title, ewmh_txt_prop.strings, ewmh_txt_prop.strings_len, 0, 1);
		}
		else if (icccm_txt_prop.name != NULL && icccm_txt_prop.name_len > 0) {
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
			wklog("could not capture property change events on window 0x%X", win);
		}
	}
	else {
		strcpy(window_title, MISSING_VALUE);
	}

	char *json_payload;
	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "window_title", json_string(window_title));

	json_payload = json_dumps(json_data_object, 0);

	widget->data = strdup(json_payload);
	g_idle_add((GSourceFunc)update_widget, widget);
	json_decref(json_data_object);
}

void
*widget_window_title (struct widget *widget) {
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		wklog("Could not connect to display %s.", getenv("DISPLAY"));
		return 0;
	}

	int screen_nbr = 0; // FIXME load from config
	xcb_window_t cur_win = 0;
	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_generic_event_t *evt;
	xcb_generic_error_t *err = xcb_request_check(ewmh->connection,
	                                             xcb_change_window_attributes_checked(ewmh->connection,
	                                                                                  ewmh->screens[screen_nbr]->root,
	                                                                                  XCB_CW_EVENT_MASK,
	                                                                                  values));

	if (err != NULL) {
		wklog("desktops: could not request EWMH property change notifications");
		return 0;
	}

	window_title_send_update(widget, ewmh, screen_nbr, &cur_win);

	for (;;) {
		while ((evt = xcb_wait_for_event(ewmh->connection)) != NULL) {
			xcb_property_notify_event_t *pne;
			switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
			case XCB_PROPERTY_NOTIFY:
				pne = (xcb_property_notify_event_t *) evt;
				if (pne->atom == ewmh->_NET_ACTIVE_WINDOW) {
					window_title_send_update(widget, ewmh, screen_nbr, &cur_win);
				}
				else if (pne->window != ewmh->screens[screen_nbr]->root && (pne->atom == ewmh->_NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
					window_title_send_update(widget, ewmh, screen_nbr, &cur_win);
				}
			default:
				break;
			}
			free(evt);
		}
	}

	xcb_ewmh_connection_wipe(ewmh);
	return 0;
}
