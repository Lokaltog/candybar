#include "widgets.h"
#include "now_playing_mpris.h"

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "player_name", config.player_name);

	gchar *player_name = (gchar *)config.player_name;
	PlayerctlPlayer *player = NULL;

	widget_epoll_init(widget);
	while (true) {
		if (!player)
			player = playerctl_player_new(player_name, NULL);

		if (player) {
			char *artist = playerctl_player_get_artist(player, NULL);
			char *title = playerctl_player_get_title(player, NULL);

			if (artist || title)
				widget_data_callback(widget, widget_data_arg_string(artist), widget_data_arg_string(title));

			g_free(artist);
			g_free(title);
		}

		widget_epoll_wait_goto(widget, 1, cleanup);
	}

cleanup:
	g_object_unref(player);

	return 0;
}
