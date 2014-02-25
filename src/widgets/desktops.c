void
*widget_desktops (thread_data_t *thread_data) {
	unsigned short i;
	json_t *json_base_object;
	json_t *json_desktop_object;
	json_t *json_desktops_array;
	char *json_payload;

	for (;;) {
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

		// TODO monitor instead
		sleep(2);
	}
}
