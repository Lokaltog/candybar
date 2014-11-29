#include "widgets.h"
#include "now_playing_mpris.h"

static int
hide_widget (JSContextRef ctx, JSObjectRef this) {
	struct js_callback_arg arg = widget_data_arg_string("hide");

	JSStringRef str = JSStringCreateWithUTF8CString(arg.value.string);
	JSValueRef refs[2] = {JSValueMakeString(ctx, str), JSValueMakeString(ctx, str)};
	JSStringRelease(str);

	JSStringRef str_ondatachanged = JSStringCreateWithUTF8CString("onDataChanged");
	JSValueRef func = JSObjectGetProperty(ctx, this, str_ondatachanged, NULL);
	JSObjectRef function = JSValueToObject(ctx, func, NULL);
	JSStringRelease(str_ondatachanged);

	JSObjectCallAsFunction(ctx, function, NULL, 2, refs, NULL);

	return 0;
}

static JSValueRef
widget_js_func_toggle (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	PlayerctlPlayer *player = playerctl_player_new(NULL, NULL);
	if (player) {
		playerctl_player_play_pause(player, NULL);
		g_object_unref(player);
	} else {
		hide_widget(ctx, this);
	}

	return JSValueMakeUndefined(ctx);
}

static JSValueRef
widget_js_func_next (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	PlayerctlPlayer *player = playerctl_player_new(NULL, NULL);
	if (player) {
		playerctl_player_next(player, NULL);
		g_object_unref(player);
	} else {
		hide_widget(ctx, this);
	}

	return JSValueMakeUndefined(ctx);
}

static JSValueRef
widget_js_func_previous (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	PlayerctlPlayer *player = playerctl_player_new(NULL, NULL);
	if (player) {
		playerctl_player_previous(player, NULL);
		g_object_unref(player);
	} else {
		hide_widget(ctx, this);
	}

	return JSValueMakeUndefined(ctx);
}

static JSValueRef
widget_js_func_status (JSContextRef ctx, JSObjectRef func, JSObjectRef this, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
	PlayerctlPlayer *player = playerctl_player_new(NULL, NULL);
	char *raw;
	double status;

	g_object_get(player, "status", &raw);
	if (strcmp(raw, "Playing") == 0) {
		status = 0;
	}
	else {
		status = 1;
	}

	g_object_unref(player);

	return JSValueMakeNumber(ctx, status);
}

const JSStaticFunction widget_js_staticfuncs[] = {
	{ "toggle", widget_js_func_toggle, kJSPropertyAttributeReadOnly },
	{ "next", widget_js_func_next, kJSPropertyAttributeReadOnly },
	{ "previous", widget_js_func_previous, kJSPropertyAttributeReadOnly },
	{ "status", widget_js_func_status, kJSPropertyAttributeReadOnly },
	{ NULL, NULL, 0 },
};

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

static void
on_status (PlayerctlPlayer *player, gpointer user_data) {
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
			g_signal_connect(player, "pause", G_CALLBACK(on_status), widget);
			g_signal_connect(player, "play", G_CALLBACK(on_status), widget);

			widget_epoll_wait_goto(widget, -1, cleanup);
		}
		else {
			/* keep trying until we find a player */
			widget_epoll_wait_goto(widget, 1, cleanup);
		}
	}

cleanup:
	g_object_unref(player);

	widget_epoll_cleanup(widget);
	widget_clean_exit(widget);
}
