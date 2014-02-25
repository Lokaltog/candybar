#include "desktops.h"

static bool
ewmh_get_active_window_name(xcb_ewmh_connection_t *ewmh, int screen_nbr, char *window_name) {
	xcb_window_t active_window;
	xcb_ewmh_get_utf8_strings_reply_t window_data;
	size_t len;

	if (! xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window_unchecked(ewmh, screen_nbr), &active_window, NULL)) {
		fprintf(stderr, "Found no active window\n");
		return false;
	}

	if (! xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name_unchecked(ewmh, active_window), &window_data, NULL)) {
		fprintf(stderr, "Could not read WM_NAME from active window\n");
		return false;
	}

	len = MIN(PROPERTY_MAX_LEN, window_data.strings_len + 1);
	memcpy(window_name, window_data.strings, len);
	window_name[len] = 0;

	xcb_ewmh_get_utf8_strings_reply_wipe(&window_data);

	return true;
}

static bool
ewmh_get_desktop_list(xcb_ewmh_connection_t *ewmh, int screen_nbr, desktop_t *desktops) {
	unsigned short i;
	uint32_t desktop_curr, desktop_len, client_desktop;
	xcb_ewmh_get_windows_reply_t clients;

	// get current desktop
	if (! xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop_unchecked(ewmh, screen_nbr), &desktop_curr, NULL)) {
		fprintf(stderr, "Could not get current desktop\n");
		return false;
	}

	// get desktop count
	if (! xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops_unchecked(ewmh, screen_nbr), &desktop_len, NULL)) {
		fprintf(stderr, "Could not get desktop count\n");
		return false;
	}

	for (i = 0; i < desktop_len; i++) {
		desktops[i].is_selected = i == desktop_curr;
		desktops[i].is_valid = true;
		desktops[i].clients_len = 0;
	}

	// get clients
	if (! xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list_unchecked(ewmh, screen_nbr), &clients, NULL)) {
		fprintf(stderr, "Could not get client list\n");
		return false;
	}

	for (i = 0; i < clients.windows_len; i++) {
		if (! xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop_unchecked(ewmh, clients.windows[i]), &client_desktop, NULL)) {
			continue;
		}
		desktops[client_desktop].clients_len++;
		// TODO check urgent hint and assign to desktop
	}

	xcb_ewmh_get_windows_reply_wipe(&clients);

	return true;
}

void
*widget_desktops (thread_data_t *thread_data) {
	unsigned short i;
	json_t *json_base_object;
	json_t *json_desktop_object;
	json_t *json_desktops_array;
	char *json_payload;

	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_generic_error_t *err = xcb_request_check(thread_data->ewmh->conn->connection, xcb_change_window_attributes_checked(thread_data->ewmh->conn->connection, thread_data->ewmh->conn->screens[thread_data->ewmh->screen_nbr]->root, XCB_CW_EVENT_MASK, values));
	xcb_generic_event_t *evt;

	if (err != NULL) {
		fprintf(stderr, "Could not request EWMH property change notifications\n");
		return 0;
	}

	for (;;) {
		while ((evt = xcb_wait_for_event(thread_data->ewmh->conn->connection)) != NULL) {
			// FIXME probably not thread safe?
			ewmh_get_active_window_name(thread_data->ewmh->conn, thread_data->ewmh->screen_nbr, thread_data->active_window_name);
			ewmh_get_desktop_list(thread_data->ewmh->conn, thread_data->ewmh->screen_nbr, thread_data->desktops);

			json_base_object = json_object();
			json_desktop_object = json_object();
			json_desktops_array = json_array();

			json_object_set_new(json_base_object, "widget", json_string("desktop"));
			json_object_set_new(json_base_object, "data", json_desktop_object);
			json_object_set_new(json_desktop_object, "current_window", json_string(thread_data->active_window_name));
			json_object_set_new(json_desktop_object, "desktops", json_desktops_array);

			for (i = 0; i < DESKTOP_MAX_LEN; i++) {
				if (! thread_data->desktops[i].is_valid) {
					break;
				}

				json_t *json_desktop = json_object();
				json_object_set_new(json_desktop, "clients_len", json_integer(thread_data->desktops[i].clients_len));
				json_array_append_new(json_desktops_array, json_desktop);

				if (thread_data->desktops[i].is_selected) {
					json_object_set_new(json_desktop_object, "current_desktop", json_integer(i));
				}
			}

			json_payload = json_dumps(json_base_object, 0);

			// inject data
			g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);

			free(evt);
		}
	}
}
