#include "widgets.h"
#include "util/log.h"

gboolean
update_widget (struct widget *widget) {
	char *script_template = "if(typeof widgets!=='undefined'){try{widgets.update('%s',%s)}catch(e){console.log('Could not update widget: '+e)}}";
	int script_length = 0;
	char *script;

	/* Get the length of the script payload. */
	script_length = snprintf(NULL,
	                         0,
	                         script_template,
	                         widget->name,
	                         widget->data);

	/* Add 1 for \0 */
	script = malloc(script_length + 1);

#ifdef DEBUG_JSON
	wklog("Updating widget %s: %s", widget->name, widget->data);
#endif
	sprintf(script, script_template, widget->name, widget->data);

	webkit_web_view_execute_script(widget->web_view, script);
	free(script);

	return FALSE; /* only run once */
}

GThread*
spawn_widget (WebKitWebView *web_view, json_t *config, const char *name) {
	widget_init_func widget_init;
	char libname[64];
	snprintf(libname, 64, "libwidget_%s", name);
	gchar *libpath = g_module_build_path(LIBDIR, libname);
	GModule *lib = g_module_open(libpath, G_MODULE_BIND_LOCAL);

	if (!g_module_symbol(lib, "widget_init", (gpointer*)&widget_init)) {
		wklog("loading of '%s' (%s) failed", libpath, name);

		return NULL;
	}

	struct widget *widget = malloc(sizeof(struct widget));

	widget->config = config;
	widget->web_view = web_view;

	/* Dont forget to free this one */
	widget->name = strdup(name);

	wklog("creating thread for widget '%s'", name);

	return g_thread_new(name, (GThreadFunc)widget_init, widget);
}

void
window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data) {
	json_t *config = user_data;
	json_t *widgets_arr = json_object_get(config, "widgets");
	size_t widgets_len = json_array_size(widgets_arr);
	GThread **widget_threads = malloc(widgets_len * sizeof(GThread));
	unsigned short i;

	wklog("webkit: window object cleared, loading widgets");

	/* TODO gracefully kill widget threads here */

	for (i = 0; i < widgets_len; i++) {
		widget_threads[i] = spawn_widget(web_view, config, json_string_value(json_array_get(widgets_arr, i)));
	}
}
