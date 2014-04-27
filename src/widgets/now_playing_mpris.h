#include <playerctl/playerctl.h>

static struct widget_config {
	const char *player_name;
} widget_config_defaults = {
	.player_name = NULL
};
