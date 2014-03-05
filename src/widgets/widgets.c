#include "widgets.h"
#include "../wkline.h" // required for struct wkline

GThread *widget_threads[LENGTH(wkline_widgets)];

gboolean
update_widget (struct widget *widget) {
	char *script_template = "if(typeof widgets!=='undefined'){try{widgets.update('%s',%s)}catch(e){console.log('Could not update widget: '+e)}}";
	int script_length = 0;
	char *script;

	// Get the length of the script payload.
	script_length = snprintf(NULL,
	                         0,
	                         script_template,
	                         widget->name,
	                         widget->data);
	// Add 1 for \0
	script = malloc(script_length + 1);

#ifdef DEBUG_JSON
	wklog("Updating widget %s: %s", widget->name, widget->data);
#endif
	sprintf(script, script_template, widget->name, widget->data);

	webkit_web_view_execute_script(widget->web_view, script);
	free(script);

	return FALSE; // only run once
}

void
window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, gpointer context, gpointer window_object, gpointer user_data) {
	struct wkline *wkline = user_data;
	unsigned short i;

	wklog("webkit: window object cleared");
	for (i = 0; i < LENGTH(wkline_widgets); i++) {
		// FIXME this is pretty bad, it should probably join and recreate the threads instead
		if (! widget_threads[i]) {
			struct widget *widget = malloc(sizeof(struct widget));

			widget->config = wkline->config;
			widget->web_view = web_view;
			// Dont forget to free this one
			widget->name = strdup(wkline_widgets[i].name);

			wklog("creating thread for widget '%s'", wkline_widgets[i].name);
			widget_threads[i] = g_thread_new(wkline_widgets[i].name, (GThreadFunc)wkline_widgets[i].func, widget);
		}
	}
}
