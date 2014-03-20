#include "widgets.h"
#include "desktops.h"

static JSValueRef
widget_js_func_set_desktop (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	if (!argc) {
		LOG_WARN("set_desktop: requires at least one argument");
		goto cleanup;
	}
	if (!JSValueIsNumber(ctx, argv[0])) {
		LOG_WARN("set_desktop: argument 1 must be a number");
		goto cleanup;
	}

	int desktop = JSValueToNumber(ctx, argv[0], NULL);

	int screen_nbr = 0;
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_connection_has_error(conn)) {
		LOG_ERR("could not connect to display %s", getenv("DISPLAY"));
		goto cleanup;
	}
	xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

	xcb_ewmh_request_change_current_desktop(ewmh, screen_nbr, desktop, XCB_CURRENT_TIME);

cleanup:
	if (ewmh != NULL) {
		xcb_ewmh_connection_wipe(ewmh);
	}
	if (conn != NULL) {
		xcb_disconnect(conn);
	}

	return JSValueMakeUndefined(ctx);
}

const JSStaticFunction widget_js_staticfuncs[] = {
	{ "set_desktop", widget_js_func_set_desktop, kJSPropertyAttributeReadOnly },
	{ NULL, NULL, 0 },
};

static int
widget_update (struct widget *widget, xcb_ewmh_connection_t *ewmh, int screen_nbr) {
	unsigned short i;
	uint32_t desktop_curr, desktop_len, client_desktop;
	char desktop_name[COPY_PROP_BUFSIZ];
	xcb_ewmh_get_utf8_strings_reply_t desktop_names;
	xcb_ewmh_get_windows_reply_t clients;
	xcb_icccm_wm_hints_t window_hints;
	struct desktop *desktops;

	/* get current desktop */
	int desktop_curr_success = xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop_unchecked(ewmh, screen_nbr), &desktop_curr, NULL);
	if (!desktop_curr_success) {
		LOG_DEBUG("ewmh: could not get current desktop");

		return 1;
	}

	/* get desktop count */
	int desktop_len_success = xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops_unchecked(ewmh, screen_nbr), &desktop_len, NULL);
	if (!desktop_len_success) {
		LOG_DEBUG("ewmh: could not get desktop count");

		return 2;
	}

	desktops = calloc(desktop_len, sizeof(struct desktop));

	int desktop_names_success = xcb_ewmh_get_desktop_names_reply(ewmh, xcb_ewmh_get_desktop_names_unchecked(ewmh, screen_nbr), &desktop_names, NULL);
	if (!desktop_names_success) {
		LOG_DEBUG("ewmh: could not get desktop names");
	}

	for (i = 0; i < desktop_len; i++) {
		desktops[i].is_selected = i == desktop_curr;
		desktops[i].is_urgent = false;
		desktops[i].clients_len = 0;

		if (desktop_names_success && desktop_names.strings) {
			copy_prop(desktop_name, desktop_names.strings, desktop_names.strings_len, i, desktop_len);
		}
		else {
			snprintf(desktop_name, COPY_PROP_BUFSIZ - 1, "%i", i + 1);
		}
		desktops[i].name = strndup(desktop_name, strlen(desktop_name));
	}

	/* get clients */
	int clients_success = xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list_unchecked(ewmh, screen_nbr), &clients, NULL);
	if (!clients_success) {
		LOG_DEBUG("ewmh: could not get client list");
	}
	else {
		for (i = 0; i < clients.windows_len; i++) {
			if (!xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop_unchecked(ewmh, clients.windows[i]), &client_desktop, NULL)) {
				/* window isn't associated with a desktop */
				continue;
			}
			desktops[client_desktop].clients_len++;

			/* check icccm urgency hint on client */
			if (!xcb_icccm_get_wm_hints_reply(ewmh->connection, xcb_icccm_get_wm_hints_unchecked(ewmh->connection, clients.windows[i]), &window_hints, NULL)) {
				LOG_DEBUG("icccm: could not get window hints");
			}
			if (window_hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY) {
				desktops[client_desktop].is_urgent = true;
			}
		}
	}

	json_t *json_data_object = json_object();
	json_t *json_desktops_array = json_array();
	json_object_set_new(json_data_object, "desktops", json_desktops_array);

	for (i = 0; i < desktop_len; i++) {
		json_t *json_desktop = json_object();
		json_object_set_new(json_desktop, "name", json_string(desktops[i].name));
		json_object_set_new(json_desktop, "clients_len", json_integer(desktops[i].clients_len));
		json_object_set_new(json_desktop, "is_urgent", json_boolean(desktops[i].is_urgent));
		json_array_append_new(json_desktops_array, json_desktop);

		if (desktops[i].is_selected) {
			json_object_set_new(json_data_object, "current_desktop", json_integer(i));
		}
	}

	char *json_str = strdup(json_dumps(json_data_object, 0));
	widget_data_callback(widget, widget_data_arg_string(json_str));

	json_decref(json_data_object);
	free(json_str);

	/* cleanup */
	if (desktop_names_success) {
		xcb_ewmh_get_utf8_strings_reply_wipe(&desktop_names);
	}
	if (clients_success) {
		xcb_ewmh_get_windows_reply_wipe(&clients);
	}
	for (i = 0; i < desktop_len; i++) {
		free(desktops[i].name);
	}
	free(desktops);

	return 0;
}

void*
widget_main (struct widget *widget) {
	unsigned short i;
	int xcb_fd;
	int screen_nbr = 0;
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

	widget_update(widget, ewmh, screen_nbr);
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
					if (pne->atom == ewmh->_NET_DESKTOP_NAMES) {
						widget_update(widget, ewmh, screen_nbr);
					}
					else if (pne->atom == ewmh->_NET_NUMBER_OF_DESKTOPS) {
						widget_update(widget, ewmh, screen_nbr);
					}
					else if (pne->atom == ewmh->_NET_CURRENT_DESKTOP) {
						widget_update(widget, ewmh, screen_nbr);
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
