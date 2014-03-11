#include "widgets.h"
#include "util/config.h"
#include "util/log.h"

static pthread_t *widget_threads = NULL;
static size_t widgets_len = 0;

gboolean
update_widget (struct widget *widget) {
	char *script_template = "if(typeof widgets!=='undefined'){try{widgets.update('%s',%s)}catch(e){console.log('Could not update widget: '+e)}}";
	int script_length = 0;
	char *script;

	script_length = snprintf(NULL, 0, script_template, widget->name, widget->data);
	script = malloc(script_length + 1);

	if (widget_get_config_boolean_silent(widget, "debug")) {
		LOG_INFO("updating widget %s: %s", widget->name, widget->data);
	}

	sprintf(script, script_template, widget->name, widget->data);

	webkit_web_view_execute_script(widget->web_view, script);
	free(script);

	return FALSE; /* only run once */
}

pthread_t
spawn_widget (WebKitWebView *web_view, json_t *config, const char *name) {
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

	widget->config = config;
	widget->web_view = web_view;
	widget->name = strdup(name); /* don't forget to free this one */

	LOG_INFO("spawning widget '%s'", name);

	pthread_create(&return_thread, NULL, (void*)widget_init, widget);
	pthread_setname_np(return_thread, name);

	return return_thread;
}

void
handle_interrupt (int signal) {
	unsigned short i;
	if ((signal == SIGTERM) || (signal == SIGINT) || (signal == SIGHUP)) {
		if (widget_threads && (widgets_len > 0)) {
			LOG_INFO("handle_interrupt: stopping widget threads");
			for (i = 0; i < widgets_len; i++) {
				if (widget_threads[i]) {
					pthread_cancel(widget_threads[i]);
				}
			}
		}
		gtk_main_quit();
	}
}

void
window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data) {
	json_t *config = user_data;
	json_t *widget_config, *widgets = json_object_get(config, "widgets");
	const char *name;
	unsigned short i;

	LOG_INFO("webkit: window object cleared");

	if (widget_threads && (widgets_len > 0)) {
		LOG_INFO("webkit: stopping running widget threads");
		for (i = 0; i < widgets_len; i++) {
			/* this call may fail if the thread never enters the
			   main thread loop, e.g. if it fails to connect to a
			   server */
			if (widget_threads[i]) {
				pthread_cancel(widget_threads[i]);
			}
		}
	}

	widgets_len = json_object_size(widgets);
	widget_threads = malloc(widgets_len * sizeof(pthread_t));

	LOG_INFO("spawning %i widget threads", widgets_len);

	i = 0;
	json_object_foreach(widgets, name, widget_config) {
		widget_threads[i++] = spawn_widget(web_view, widget_config, name);
	}
}
