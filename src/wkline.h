typedef struct WklineDimensions {
	int w;
	int h;
} WklineDimensions;

typedef struct WklineThreadData {
	WebKitWebView *web_view;
	char *buf;
} WklineThreadData;

static void fifo_monitor (gpointer data);
static int get_intern_atom (xcb_connection_t *conn, char *atom);
static gboolean web_view_inject (gpointer data);
static gboolean wk_context_menu_cb (WebKitWebView *web_view, GtkWidget *window);
static void wk_notify_load_status_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);
static void wk_notify_progress_cb (WebKitWebView *web_view, GParamSpec *pspec, GtkWidget *window);
