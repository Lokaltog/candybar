#include "widgets.h"
#include "now_playing_mpris.h"

static PlayerctlPlayer *widget_player;

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

static int
clear_widget (struct widget *widget) {
	struct js_callback_arg args[2] = {
		widget_data_arg_string(""),
		widget_data_arg_string(""),
	};

	struct js_callback_data data = {
		.widget = widget,
		.args = args,
		2,
	};

	web_view_callback(&data);

	return 0;
}

static void
player_update_callback (PlayerctlPlayer *player, GVariant *e, gpointer user_data) {
	struct widget *widget = user_data;
	update_widget(widget, player);
}

static void
player_clear_callback (PlayerctlPlayer *player, gpointer user_data) {
	struct widget *widget = user_data;
	clear_widget(widget);
}

static void
player_exit_destroy_callback (PlayerctlPlayer *player, gpointer user_data) {
	g_clear_object(&widget_player);
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "player_name", config.player_name);

	gchar *player_name = (gchar*)config.player_name;
	widget_player = NULL;

	widget_epoll_init(widget);
	while (true) {
		if (widget_player == NULL) {
			widget_player = playerctl_player_new(player_name, NULL);

			if (widget_player) {
				/* update the widget on new metadata, or on play */
				g_signal_connect(widget_player, "metadata",
						G_CALLBACK(player_update_callback), widget);
				g_signal_connect(widget_player, "play",
						G_CALLBACK(player_update_callback), widget);

				/* clear the widget on stop and exit */
				g_signal_connect(widget_player, "stop",
						G_CALLBACK(player_clear_callback), widget);
				g_signal_connect(widget_player, "exit",
						G_CALLBACK(player_clear_callback), widget);

				if (player_name != NULL) {
					/* if a player name was passed, stick with it */
					widget_epoll_wait_goto(widget, -1, cleanup);
					/* not reached */
				}
				else {
					/* otherwise, remove the player when it exits and start polling again */
					g_signal_connect(widget_player, "exit",
							G_CALLBACK(player_exit_destroy_callback), widget);
				}
			}
		}

		/* if a player name was not configured, poll for new or different players */
		widget_epoll_wait_goto(widget, 2, cleanup);
	}

cleanup:
	g_clear_object(&widget_player);

	widget_epoll_cleanup(widget);
	widget_clean_exit(widget);
}
