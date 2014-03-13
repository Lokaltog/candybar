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

		return NULL;
	}

	free(geoip_raw_json);

	geoip_city = json_object_get(location_data, "city");
	if (!json_is_string(geoip_city)) {
		LOG_ERR("received GeoIP city is not a string");

		return NULL;
	}
	geoip_region_code = json_object_get(location_data, "region_code");
	if (!json_is_string(geoip_region_code)) {
		LOG_ERR("received GeoIP region code is not a string");

		return NULL;
	}
	geoip_country_code = json_object_get(location_data, "country_code");
	if (!json_is_string(geoip_country_code)) {
		LOG_ERR("received GeoIP country code is not a string");

		return NULL;
	}

	location->city = strdup(json_string_value(geoip_city));
	location->region_code = strdup(json_string_value(geoip_region_code));
	location->country_code = strdup(json_string_value(geoip_country_code));

	json_decref(location_data);

	return location;
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
		free(weather);

		return NULL;
	}

	free(weather_raw_json);

	json_t *tmp_obj, *weather_code, *weather_temp;
	tmp_obj = json_object_get(weather_data, "query");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "results");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "weather");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "rss");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "channel");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "item");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}
	tmp_obj = json_object_get(tmp_obj, "condition");
	if (!json_is_object(tmp_obj)) {
		return NULL;
	}

	weather_code = json_object_get(tmp_obj, "code");
	weather_temp = json_object_get(tmp_obj, "temp");

	if (!json_is_string(weather_code) || !json_is_string(weather_temp)) {
		json_decref(weather_data);
		free(weather);
		LOG_ERR("weather: invalid weather query result (weather code or temp missing)");

		return NULL;
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

	json_decref(weather_data);

	return weather;
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

static void
widget_cleanup (void *arg) {
	LOG_DEBUG("cleanup");

	void **cleanup_data = arg;
	struct location *location = cleanup_data[0];

	free(location->city);
	free(location->region_code);
	free(location->country_code);
	free(location);
	free(arg);
}

void*
widget_init (struct widget *widget) {
	LOG_DEBUG("init");

	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget, "location", config.location);
	widget_init_config_string(widget, "unit", config.unit);
	widget_init_config_integer(widget, "refresh_interval", config.refresh_interval);

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
		free(location);

		return 0;
	}

	void **cleanup_data = malloc(sizeof(void*) * 1);
	cleanup_data[0] = location;

	pthread_cleanup_push(widget_cleanup, cleanup_data);
	for (;;) {
		widget_update(widget, location, config);

		sleep(config.refresh_interval);
	}
	pthread_cleanup_pop(1);
}
