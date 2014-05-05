#include "widgets.h"
#include "now_playing_mpris.h"

static int
update_widget (struct widget *widget, PlayerctlPlayer *player) {
	char *artist = playerctl_player_get_artist(player, NULL);
	char *title = playerctl_player_get_title(player, NULL);

	struct js_callback_arg args[2] = {
		widget_data_arg_string(artist),
		widget_data_arg_string(title),
	};

	struct js_callback_data data = {
		.widget = widget,
		.args = args,
		2,
	};

	web_view_callback(&data);

	g_free(artist);
	g_free(title);

	return 0;
}

static void
on_metadata (PlayerctlPlayer *player, GVariant *e, gpointer user_data) {
	struct widget *widget = user_data;
	update_widget(widget, player);
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "player_name", config.player_name);

	gchar *player_name = (gchar*)config.player_name;
	PlayerctlPlayer *player = NULL;

	widget_epoll_init(widget);
	while (true) {
		player = playerctl_player_new(player_name, NULL);

		if (player) {
			g_signal_connect(player, "metadata", G_CALLBACK(on_metadata), widget);

			widget_epoll_wait_goto(widget, -1, cleanup);
		} else {
			/* keep trying until we find a player */
			widget_epoll_wait_goto(widget, 1, cleanup);
		}
	}

cleanup:
	g_object_unref(player);

	return 0;
}
