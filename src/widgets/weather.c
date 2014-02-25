#include "weather.h"

static size_t
write_response (void *ptr, size_t size, size_t nmemb, void *stream) {
	write_result_t *result = (write_result_t *)stream;

	if (result->pos + size * nmemb >= CURL_BUF_SIZE - 1) {
		fprintf(stderr, "error: too small buffer\n");
		return 0;
	}

	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;

	return size * nmemb;
}

static char
*request (const char *url) {
	CURL *curl;
	CURLcode status;
	char *data;
	long code;

	curl = curl_easy_init();
	data = malloc(CURL_BUF_SIZE);
	if (! curl || ! data) {
		return NULL;
	}

	write_result_t write_result = {
		.data = data,
		.pos = 0
	};

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	status = curl_easy_perform(curl);
	if (status != 0) {
		fprintf(stderr, "error: unable to request data from %s:\n", url);
		fprintf(stderr, "%s\n", curl_easy_strerror(status));
		return NULL;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		fprintf(stderr, "error: server responded with code %ld\n", code);
		return NULL;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	data[write_result.pos] = '\0';

	return data;
}

static location_t
*get_geoip_location () {
	json_t *location_data, *geoip_city, *geoip_region_code, *geoip_country_code;
	json_error_t error;
	location_t *location = malloc(sizeof(location_t));

	char *geoip_raw_json = request("http://freegeoip.net/json/");
	location_data = json_loads(geoip_raw_json, 0, &error);
	free(geoip_raw_json);

	if (! location_data) {
		fprintf(stderr, "Error while fetching GeoIP data\n");
		return NULL;
	}

	geoip_city = json_object_get(location_data, "city");
	if (! json_is_string(geoip_city)) {
		fprintf(stderr, "GeoIP city is not a string\n");
		return NULL;
	}
	geoip_region_code = json_object_get(location_data, "region_code");
	if (! json_is_string(geoip_region_code)) {
		fprintf(stderr, "GeoIP region code is not a string\n");
		return NULL;
	}
	geoip_country_code = json_object_get(location_data, "country_code");
	if (! json_is_string(geoip_country_code)) {
		fprintf(stderr, "GeoIP country code is not a string\n");
		return NULL;
	}

	location->city = strdup(json_string_value(geoip_city));
	location->region_code = strdup(json_string_value(geoip_region_code));
	location->country_code = strdup(json_string_value(geoip_country_code));

	json_decref(location_data);

	return location;
}

static weather_t
*get_weather_information (location_t *location, const char *unit) {
	char request_uri[WEATHER_BUF_SIZE];
	char query[WEATHER_BUF_SIZE];
	json_t *weather_data;
	json_error_t error;
	weather_t *weather = malloc(sizeof(weather_t));
	CURL *curl;

	curl = curl_easy_init();
	sprintf(query, "use \"http://github.com/yql/yql-tables/raw/master/weather/weather.bylocation.xml\" as we;select * from we where location=\"%s, %s, %s\" and unit=\"%s\"", location->city, location->region_code, location->country_code, unit);
	sprintf(request_uri, "http://query.yahooapis.com/v1/public/yql?q=%s&format=json", curl_easy_escape(curl, query, 0));
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	char *weather_raw_json = request(request_uri);
	weather_data = json_loads(weather_raw_json, 0, &error);
	free(weather_raw_json);

	if (! weather_data) {
		fprintf(stderr, "Error while fetching weather data\n");
		return NULL;
	}

	json_t *tmp_obj, *weather_code, *weather_temp;
	tmp_obj = json_object_get(weather_data, "query");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "results");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "weather");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "rss");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "channel");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "item");
	if (! json_is_object(tmp_obj)) return NULL;
	tmp_obj = json_object_get(tmp_obj, "condition");
	if (! json_is_object(tmp_obj)) return NULL;

	weather_code = json_object_get(tmp_obj, "code");
	weather_temp = json_object_get(tmp_obj, "temp");

	sscanf(json_string_value(weather_code), "%u", &weather->code);
	sscanf(json_string_value(weather_temp), "%lf", &weather->temp);
	strcpy(weather->unit, unit);

	json_decref(weather_data);

	return weather;
}

void
*widget_weather (thread_data_t *thread_data) {
	json_t *json_base_object;
	json_t *json_data_object;
	char *json_payload;
	location_t *location = malloc(sizeof(location_t));

	for (;;) {
		json_base_object = json_object();
		json_data_object = json_object();

		json_object_set_new(json_base_object, "widget", json_string("weather"));
		json_object_set_new(json_base_object, "data", json_data_object);

		// get location
		if (wkline_widget_weather_location[0] == '\0') {
			location = get_geoip_location();
		}
		else {
			location->city = strdup(wkline_widget_weather_location);
		}

		if (! location) {
			free(location);
			goto next_iter;
		}

		weather_t *weather = get_weather_information(location, wkline_widget_weather_unit);

		if (! weather) {
			free(location);
			free(weather);
			goto next_iter;
		}

		json_object_set_new(json_data_object, "icon", json_integer(weather->code));
		json_object_set_new(json_data_object, "temp", json_real(weather->temp));
		json_object_set_new(json_data_object, "unit", json_string(weather->unit));

		free(location->city);
		free(location->region_code);
		free(location->country_code);
		free(location);
		free(weather);

		json_payload = json_dumps(json_base_object, 0);

		// inject data
		g_idle_add((GSourceFunc)wk_web_view_inject, json_payload);

	next_iter:
		sleep(600);
	}
}
