typedef struct location_t {
	char *city;
	char *region_code;
	char *country_code;
} location_t;

typedef struct weather_t {
	int code;
	double temp;
	char unit[1];
} weather_t;

static location_t *get_geoip_location ();
static weather_t *get_weather_information (location_t *location, const char *unit);
