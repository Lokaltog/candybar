#define PROPERTY_MAX_LEN 256
#define DESKTOP_MAX_LEN 10

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

typedef struct desktop_t {
	bool is_selected;
	bool is_urgent;
	bool is_valid;
	int clients_len;
} desktop_t;

typedef struct ewmh_data_t {
	int screen_nbr;
	xcb_ewmh_connection_t *conn;
} ewmh_data_t;

typedef struct thread_data_t {
	desktop_t desktops[DESKTOP_MAX_LEN];
	char active_window_name[PROPERTY_MAX_LEN];
	ewmh_data_t *ewmh;
} thread_data_t;

static bool ewmh_get_active_window_name(xcb_ewmh_connection_t *ewmh, int screen_nbr, char *window_name);
static bool ewmh_get_desktop_list(xcb_ewmh_connection_t *ewmh, int screen_nbr, desktop_t *desktops);
static gboolean wk_web_view_inject (char *payload);
static gboolean wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window);
static void wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);
