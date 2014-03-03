#include "widgets.h"
#include "desktops.h"

xcb_window_t cur_win = 0;
xcb_ewmh_connection_t *ewmh;
int screen_nbr;
desktop_t *desktops;
char window_title[BUFSIZ];

void
copy_prop (char *dest, char *src, int len, int idx, int num_itm)
{
	if (num_itm <= 1) {
		strncpy(dest, src, MIN(len, BUFSIZ));
		dest[len] = '\0';
	}
	else {
		int pos = 0, cnt = 0;
		while (cnt < idx && cnt < (num_itm - 1) && pos < len) {
			pos += strlen(src + pos) + 1;
			cnt++;
		}
		if (cnt == (num_itm - 1)) {
			copy_prop(dest, src + pos, len - pos, 0, 1);
		}
		else {
			strncpy(dest, src + pos, BUFSIZ);
		}
	}
}

static void
desktops_send_update () {
	unsigned short i;
	uint32_t desktop_curr, desktop_len, client_desktop;
	xcb_ewmh_get_windows_reply_t clients;
	xcb_icccm_wm_hints_t window_hints;

	desktops = calloc(DESKTOP_MAX_LEN, sizeof(desktop_t));

	// get current desktop
	if (! xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop_unchecked(ewmh, screen_nbr), &desktop_curr, NULL)) {
		wklog("ewmh: could not get current desktop");
		return;
	}

	// get desktop count
	if (! xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops_unchecked(ewmh, screen_nbr), &desktop_len, NULL)) {
		wklog("ewmh: could not get desktop count");
		return;
	}

	for (i = 0; i < desktop_len; i++) {
		desktops[i].is_selected = i == desktop_curr;
		desktops[i].is_valid = true;
		desktops[i].is_urgent = false;
		desktops[i].clients_len = 0;
	}

	// get clients
	if (! xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list_unchecked(ewmh, screen_nbr), &clients, NULL)) {
		wklog("ewmh: could not get client list");
		return;
	}

	for (i = 0; i < clients.windows_len; i++) {
		if (! xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop_unchecked(ewmh, clients.windows[i]), &client_desktop, NULL)) {
			// window isn't associated with a desktop
			continue;
		}
		desktops[client_desktop].clients_len++;

		// check icccm urgency hint on client
		if (! xcb_icccm_get_wm_hints_reply(ewmh->connection, xcb_icccm_get_wm_hints_unchecked(ewmh->connection, clients.windows[i]), &window_hints, NULL)) {
			wklog("icccm: could not get window hints");
		}
		if (window_hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY) {
			desktops[client_desktop].is_urgent = true;
		}
	}

	xcb_ewmh_get_windows_reply_wipe(&clients);

	json_t *json_data_object = json_object();
	json_t *json_desktops_array = json_array();
	char *json_payload;

	for (i = 0; i < DESKTOP_MAX_LEN; i++) {
		if (! desktops[i].is_valid) {
			continue;
		}

		json_t *json_desktop = json_object();
		json_object_set_new(json_desktop, "clients_len", json_integer(desktops[i].clients_len));
		json_object_set_new(json_desktop, "is_urgent", json_boolean(desktops[i].is_urgent));
		json_array_append_new(json_desktops_array, json_desktop);

		if (desktops[i].is_selected) {
			json_object_set_new(json_data_object, "current_desktop", json_integer(i));
		}
	}

	json_object_set_new(json_data_object, "desktops", json_desktops_array);

	json_payload = json_dumps(json_data_object, 0);

	widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
	widget_data->widget = "desktops";
	widget_data->data = json_payload;
	g_idle_add((GSourceFunc)update_widget, widget_data);

	free(desktops);
}

static void
window_title_send_update () {
	xcb_window_t win;
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	xcb_icccm_get_text_property_reply_t icccm_txt_prop;
	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	uint32_t values_reset[] = {XCB_EVENT_MASK_NO_EVENT};

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
		if (win != cur_win) {
			xcb_change_window_attributes(ewmh->connection, cur_win, XCB_CW_EVENT_MASK, values_reset);
			cur_win = win;
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

	widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
	widget_data->widget = "window_title";
	widget_data->data = json_payload;
	g_idle_add((GSourceFunc)update_widget, widget_data);
}

void
*widget_desktops () {
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		wklog("Could not connect to display %s.", getenv("DISPLAY"));
		return 0;
	}

	/* FIXME ewmh should not be a global */
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
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

	desktops_send_update();
	window_title_send_update();

	for (;;) {
		while ((evt = xcb_wait_for_event(ewmh->connection)) != NULL) {
			xcb_property_notify_event_t *pne;
			switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
			case XCB_PROPERTY_NOTIFY:
				pne = (xcb_property_notify_event_t *) evt;
				if (pne->atom == ewmh->_NET_DESKTOP_NAMES) {
					desktops_send_update();
				}
				else if (pne->atom == ewmh->_NET_ACTIVE_WINDOW) {
					window_title_send_update();
				}
				else if (pne->window != ewmh->screens[screen_nbr]->root && (pne->atom == ewmh->_NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
					window_title_send_update();
				}
				else if (pne->atom == ewmh->_NET_NUMBER_OF_DESKTOPS) {
					desktops_send_update();
				}
				else if (pne->atom == ewmh->_NET_CURRENT_DESKTOP) {
					desktops_send_update();
				}
			default:
				break;
			}
			free(evt);
		}
	}
	return 0;
}
