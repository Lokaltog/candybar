#include <poll.h>
#include <mpd/client.h>

static int widget_now_playing_mpd_send_update (struct mpd_connection *connection);
