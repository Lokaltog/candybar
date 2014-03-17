#include "widgets.h"
#include "weather.h"

static struct location*
get_geoip_location (struct location *location) {
	json_t *location_data, *geoip_city, *geoip_region_code, *geoip_country_code;
	json_error_t error;

	char *geoip_raw_json = wkline_curl_request("http://freegeoip.net/json/");
	location_data = json_loads(geoip_raw_json, 0, &error);

	if (!location_data) {
		LOG_WARN("error while fetching GeoIP data");
		goto error;
	}

	free(geoip_raw_json);

	geoip_city = json_object_get(location_data, "city");
	if (!json_is_string(geoip_city)) {
		LOG_ERR("received GeoIP city is not a string");
		goto error;
	}
	geoip_region_code = json_object_get(location_data, "region_code");
	if (!json_is_string(geoip_region_code)) {
		LOG_ERR("received GeoIP region code is not a string");
		goto error;
	}
	geoip_country_code = json_object_get(location_data, "country_code");
	if (!json_is_string(geoip_country_code)) {
		LOG_ERR("received GeoIP country code is not a string");
		goto error;
	}

	location->city = strdup(json_string_value(geoip_city));
	location->region_code = strdup(json_string_value(geoip_region_code));
	location->country_code = strdup(json_string_value(geoip_country_code));

	if (location_data != NULL) {
		json_decref(location_data);
	}

	return location;

error:
	if (geoip_raw_json != NULL) {
		free(geoip_raw_json);
	}
	if (location_data != NULL) {
		json_decref(location_data);
	}

	return NULL;
}

static struct weather*
get_weather_information (struct location *location) {
	int query_str_len, request_uri_len;
	char *query_str, *query_str_escaped, *request_uri;
	char *query_str_template = "use \"http://github.com/yql/yql-tables/raw/master/weather/weather.bylocation.xml\" as we;"
	                           "select * from we where location=\"%s %s %s\" and unit=\"c\"";
	char *request_uri_template = "http://query.yahooapis.com/v1/public/yql?q=%s&format=json";
	json_t *weather_data;
	json_error_t error;
	struct weather *weather = calloc(1, sizeof(struct weather));
	CURL *curl;

	curl = curl_easy_init();

	query_str_len = snprintf(NULL, 0, query_str_template, location->city, location->region_code, location->country_code);
	query_str = malloc(query_str_len + 1);
	snprintf(query_str, query_str_len + 1, query_str_template, location->city, location->region_code, location->country_code);

	query_str_escaped = curl_easy_escape(curl, query_str, 0);
	request_uri_len = snprintf(NULL, 0, request_uri_template, query_str_escaped);
	request_uri = malloc(request_uri_len + 1);
	snprintf(request_uri, request_uri_len + 1, request_uri_template, query_str_escaped);

	char *weather_raw_json = wkline_curl_request(request_uri);
	weather_data = json_loads(weather_raw_json, 0, &error);

	free(query_str);
	free(query_str_escaped);
	free(request_uri);
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	if (!weather_data) {
		goto error;
	}

	free(weather_raw_json);

	json_t *weather_code, *weather_temp;
	json_t *tmp_obj = NULL;
	tmp_obj = json_object_get(weather_data, "query");
	tmp_obj = json_object_get(tmp_obj, "results");
	tmp_obj = json_object_get(tmp_obj, "weather");
	tmp_obj = json_object_get(tmp_obj, "rss");
	tmp_obj = json_object_get(tmp_obj, "channel");
	tmp_obj = json_object_get(tmp_obj, "item");
	tmp_obj = json_object_get(tmp_obj, "condition");
	if (!json_is_object(tmp_obj)) {
		LOG_ERR("invalid weather data object received");
		goto error;
	}

	weather_code = json_object_get(tmp_obj, "code");
	weather_temp = json_object_get(tmp_obj, "temp");

	if (!json_is_string(weather_code) || !json_is_string(weather_temp)) {
		LOG_ERR("invalid weather query result received (weather code or temp missing)");
		if (tmp_obj != NULL) {
			json_decref(tmp_obj);
		}
		goto error;
	}

	int int_val;
	char *end;

	int_val = strtol(json_string_value(weather_code), &end, 10);
	if (*end) {
		LOG_WARN("received weather code is not an integer");
	}
	else {
		weather->code = int_val;
	}

	int_val = strtol(json_string_value(weather_temp), &end, 10);
	if (*end) {
		LOG_WARN("received temperature is not an integer");
	}
	else {
		weather->temp = int_val;
	}

	if (tmp_obj != NULL) {
		json_decref(tmp_obj);
	}
	if (weather_data != NULL) {
		json_decref(weather_data);
	}

	return weather;

error:
	if (weather_data != NULL) {
		json_decref(weather_data);
	}
	if (weather != NULL) {
		free(weather);
	}

	return NULL;
}

static int
widget_update (struct widget *widget, struct location *location, struct widget_config config) {
	struct weather *weather;

	weather = get_weather_information(location);
	if (!weather) {
		LOG_ERR("error while fetching weather data");

		return -1;
	}

	json_t *json_data_object = json_object();
	json_object_set_new(json_data_object, "icon", json_integer(weather->code));
	json_object_set_new(json_data_object, "temp", json_real(weather->temp));
	json_object_set_new(json_data_object, "unit", json_string(config.unit));

	widget_send_update(json_data_object, widget);

	free(weather);

	return 0;
}

void*
widget_init (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "location", config.location);
	widget_init_config_string(widget->config, "unit", config.unit);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	struct location *location = calloc(1, sizeof(location));

	if (strlen(config.location) > 0) {
		location->city = strdup(config.location);
		location->region_code = strdup("");
		location->country_code = strdup("");
	}
	else {
		location = get_geoip_location(location);
	}
	if (!location) {
		LOG_WARN("could not get GeoIP location, consider setting the location manually in config.json");
		goto cleanup;
	}

	widget_epoll_init(widget);
	while (true) {
		widget_update(widget, location, config);
		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:
	if (location->city != NULL) {
		free(location->city);
	}
	if (location->region_code != NULL) {
		free(location->region_code);
	}
	if (location->country_code != NULL) {
		free(location->country_code);
	}
	if (location != NULL) {
		free(location);
	}

	return 0;
}
