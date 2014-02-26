#include "desktops.h"

static int
ewmh_get_active_window_name(xcb_ewmh_connection_t *ewmh, int screen_nbr, char *window_name) {
	xcb_window_t active_window;
	xcb_ewmh_get_utf8_strings_reply_t window_data;
	size_t len;

	if (! xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window_unchecked(ewmh, screen_nbr), &active_window, NULL)) {
		wklog("ewmh: found no active window");
		return 1;
	}

	if (! xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name_unchecked(ewmh, active_window), &window_data, NULL)) {
		wklog("ewmh: could not read WM_NAME from active window");
		return 2;
	}

	len = MIN(PROPERTY_MAX_LEN, window_data.strings_len + 1);
	memcpy(window_name, window_data.strings, len);
	window_name[len] = 0;

	xcb_ewmh_get_utf8_strings_reply_wipe(&window_data);

	return 0;
}

static int
ewmh_get_desktop_list(xcb_ewmh_connection_t *ewmh, int screen_nbr, desktop_t *desktops) {
	unsigned short i;
	uint32_t desktop_curr, desktop_len, client_desktop;
	xcb_ewmh_get_windows_reply_t clients;
	xcb_icccm_wm_hints_t window_hints;

	// get current desktop
	if (! xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop_unchecked(ewmh, screen_nbr), &desktop_curr, NULL)) {
		wklog("ewmh: could not get current desktop");
		return 1;
	}

	// get desktop count
	if (! xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops_unchecked(ewmh, screen_nbr), &desktop_len, NULL)) {
		wklog("ewmh: could not get desktop count");
		return 2;
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
		return 3;
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

	return 0;
}

static int
widget_desktops_send_update () {
	unsigned short i;
	json_t *json_base_object = json_object();
	json_t *json_data_object = json_object();
	json_t *json_desktops_array = json_array();
	char *json_payload;
	char *active_window_name = malloc(PROPERTY_MAX_LEN + 1);
	desktop_t *desktops = malloc(sizeof(desktop_t) * DESKTOP_MAX_LEN);

	if (ewmh_get_active_window_name(thread_data.ewmh, thread_data.screen_nbr, active_window_name) > 0) {
		wklog("error while fetching active window name");
		return -1;
	}
	if (ewmh_get_desktop_list(thread_data.ewmh, thread_data.screen_nbr, desktops) > 0) {
		wklog("error while fetching desktop list");
		return -1;
	}

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

	json_object_set_new(json_base_object, "widget", json_string("desktop"));
	json_object_set_new(json_base_object, "data", json_data_object);
	json_object_set_new(json_data_object, "current_window", json_string(active_window_name));
	json_object_set_new(json_data_object, "desktops", json_desktops_array);

	json_payload = json_dumps(json_base_object, 0);

	// inject data
	g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);

	return 0;
}

void
*widget_desktops () {
	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_generic_event_t *evt;
	xcb_generic_error_t *err = xcb_request_check(thread_data.ewmh->connection,
	                                             xcb_change_window_attributes_checked(thread_data.ewmh->connection,
	                                                                                  thread_data.ewmh->screens[thread_data.screen_nbr]->root,
	                                                                                  XCB_CW_EVENT_MASK,
	                                                                                  values));
	if (err != NULL) {
		wklog("desktops: could not request EWMH property change notifications");
		return 0;
	}

	widget_desktops_send_update();
	for (;;) {
		while ((evt = xcb_wait_for_event(thread_data.ewmh->connection)) != NULL) {
			widget_desktops_send_update();
			free(evt);
		}
	}
	return 0;
}
