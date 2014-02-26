#define PROPERTY_MAX_LEN 256
#define DESKTOP_MAX_LEN 10

typedef struct desktop_t {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
} desktop_t;

static int ewmh_get_active_window_name(xcb_ewmh_connection_t *ewmh, int screen_nbr, char *window_name);
static int ewmh_get_desktop_list(xcb_ewmh_connection_t *ewmh, int screen_nbr, desktop_t *desktops);
static int widget_desktops_send_update ();
