void
*widget_datetime (thread_data_t *thread_data) {
	json_t *json_base_object;
	json_t *json_datetime_object;
	char *json_payload;
	time_t t;
	struct tm *tmp;
	char timestr[200];
	char datestr[200];

	for (;;) {
		t = time(NULL);
		tmp = localtime(&t);
		if (tmp == NULL) {
			perror("localtime");
			return false;
		}

		strftime(datestr, sizeof(datestr), wkline_widget_datetime_date_format, tmp);
		strftime(timestr, sizeof(timestr), wkline_widget_datetime_time_format, tmp);

		json_base_object = json_object();
		json_datetime_object = json_object();

		json_object_set_new(json_base_object, "widget", json_string("datetime"));
		json_object_set_new(json_base_object, "data", json_datetime_object);

		json_object_set_new(json_datetime_object, "date", json_string(datestr));
		json_object_set_new(json_datetime_object, "time", json_string(timestr));

		json_payload = json_dumps(json_base_object, 0);

		// inject data
		g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);

		sleep(10);
	}
}
