#include <string.h>
#include <stdbool.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

#define DESKTOP_MAX_LEN 10

struct desktop {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
};
