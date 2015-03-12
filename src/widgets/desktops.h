#include <jansson.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

struct desktop {
	char *name;
	bool is_selected;
	bool is_urgent;
	int clients_len;
};

static struct widget_config {
	bool show_empty;
} widget_config_defaults = {
	.show_empty = true,
};
