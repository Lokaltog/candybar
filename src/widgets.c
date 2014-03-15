#include "widgets.h"
#include "wkline.h"

static pthread_t *widget_threads = NULL;
static size_t widgets_len = 0;

static void
cancel_threads () {
	unsigned short i;
	if (widget_threads && (widgets_len > 0)) {
		LOG_DEBUG("stopping widget threads");
		for (i = 0; i < widgets_len; i++) {
			if (widget_threads[i]) {
				/* this call may fail if the thread never
				   entered the main thread loop */
				pthread_cancel(widget_threads[i]);
				pthread_join(widget_threads[i], NULL);
			}
		}
	}
}

gboolean
web_view_update_widget (struct widget *widget) {
	char *script_template = "if(typeof widgets!=='undefined'){"
	                        "try{widgets.update('%s',%s)}"
	                        "catch(e){console.log('Could not update widget: '+e)}}";
	int script_length = 0;
	char *script;

	script_length = snprintf(NULL, 0, script_template, widget->name, widget->data);
	script = malloc(script_length + 1);

	if (widget_get_config_boolean_silent(widget, "debug")) {
		LOG_DEBUG("updating widget %s: %s", widget->name, widget->data);
	}

	snprintf(script, script_length + 1, script_template, widget->name, widget->data);

	webkit_web_view_execute_script(widget->web_view, script);
	free(script);

	return FALSE; /* only run once */
}

pthread_t
spawn_widget (WebKitWebView *web_view, json_t *json_config, const char *name) {
	widget_init_func widget_init;
	char libname[64];
	snprintf(libname, 64, "libwidget_%s", name);
	gchar *libpath = g_module_build_path(LIBDIR, libname);
	GModule *lib = g_module_open(libpath, G_MODULE_BIND_LOCAL);
	pthread_t return_thread;

	if (lib == NULL) {
		LOG_WARN("loading of '%s' (%s) failed", libpath, name);

		return 0;
	}

	if (!g_module_symbol(lib, "widget_init", (gpointer*)&widget_init)) {
		LOG_WARN("loading of '%s' (%s) failed: unable to load module symbol", libpath, name);

		return 0;
	}

	struct widget *widget = malloc(sizeof(struct widget));

	widget->json_config = json_config;
	widget->web_view = web_view;
	widget->name = strdup(name); /* don't forget to free this one */

	pthread_create(&return_thread, NULL, (void*(*)(void*))widget_init, widget);

	return return_thread;
}

void
handle_interrupt (int signal) {
	if ((signal == SIGTERM) || (signal == SIGINT) || (signal == SIGHUP)) {
		cancel_threads();
		gtk_main_quit();
	}
}

void
window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data) {
	json_t *config = user_data;
	json_t *widget;
	json_t *widgets = json_object_get(config, "widgets");
	size_t index;

	LOG_DEBUG("webkit: window object cleared");

	cancel_threads();

	widgets_len = json_array_size(widgets);
	widget_threads = malloc(widgets_len * sizeof(pthread_t));

	json_array_foreach(widgets, index, widget) {
		widget_threads[index] = spawn_widget(web_view,
		                                     json_object_get(widget, "config"),
		                                     json_string_value(json_object_get(widget, "module")));
	}
}
