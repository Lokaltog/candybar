#include <stdbool.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include "util/copy_prop.h"

#define DESKTOP_MAX_LEN 10

struct desktop {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
};
