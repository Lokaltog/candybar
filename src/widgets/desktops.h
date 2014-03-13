#include <pthread.h>
#include <stdbool.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

struct desktop {
	char *name;
	bool is_selected;
	bool is_urgent;
	int clients_len;
};
