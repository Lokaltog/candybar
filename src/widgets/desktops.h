#include <string.h>
#include <stdbool.h>
#include <xcb/xcb_icccm.h>

#define PROPERTY_MAX_LEN 256
#define DESKTOP_MAX_LEN 10

typedef struct desktop_t {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
} desktop_t;
