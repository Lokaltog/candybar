#include "widgets.h"

GThread *widget_threads[LENGTH(widgets)];

gboolean
update_widget (widget_data_t *widget_data) {
	char *script_template = "if(typeof widgets!=='undefined'){try{widgets.update('%s',%s)}catch(e){console.log('Could not update widget: '+e)}}";
	int script_length = 0;
	char *script;

	// Get the length of the script payload
	script_length = snprintf(NULL,
	                         0,
	                         script_template,
	                         widget_data->widget,
	                         widget_data->data);
	// Add 1 for \0
	script = malloc(script_length + 1);

#ifdef DEBUG_JSON
	wklog("Updating widget %s: %s", widget_data->widget, widget_data->data);
#endif
	sprintf(script, script_template, widget_data->widget, widget_data->data);

	webkit_web_view_execute_script(widget_data->web_view, script);
	free(script);

	return FALSE; // only run once
}

void
window_object_cleared_cb (WebKitWebView *web_view,
                          WebKitWebFrame *frame,
                          gpointer context,
                          gpointer window_object,
                          gpointer user_data) {
	unsigned short i;

	wklog("webkit: window object cleared");
	for (i = 0; i < LENGTH(widgets); i++) {
		// FIXME this is pretty bad, it should probably join and recreate the threads instead
		if (! widget_threads[i]) {
			widget_data_t *widget_data = malloc(sizeof(widget_data_t));
			strcpy(widget_data->widget, widgets[i].id);
			widget_data->web_view = web_view;

			wklog("creating thread for widget '%s'", widgets[i].id);
			widget_threads[i] = g_thread_new(widgets[i].id, (GThreadFunc)widgets[i].func, widget_data);
		}
	}
}
