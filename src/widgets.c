#include "widgets.h"
#include "wkline.h"

static pthread_t *widget_threads = NULL;
static size_t widgets_len = 0;

static void
init_widget_js_obj (void *context, struct widget *widget) {
	const JSClassDefinition widget_js_def = { .className = widget->name };

	JSClassRef class_def = JSClassCreate(&widget_js_def);
	JSObjectRef class_obj = JSObjectMake(context, class_def, context);
	JSObjectRef global_obj = JSContextGetGlobalObject(context);
	JSStringRef str_name = JSStringCreateWithUTF8CString(widget->name);
	JSObjectSetProperty(context, global_obj, str_name, class_obj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str_name);

	widget->js_context = context;
	widget->js_object = class_obj;
}

static pthread_t
spawn_widget (struct wkline *wkline, void *context, json_t *config, const char *name) {
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

	if (!g_module_symbol(lib, "widget_init", (void*)&widget_init)) {
		LOG_WARN("loading of '%s' (%s) failed: unable to load module symbol", libpath, name);

		return 0;
	}

	struct widget *widget = malloc(sizeof(struct widget));

	widget->wkline = wkline;
	widget->config = config;
	widget->name = strdup(name); /* don't forget to free this one */

	init_widget_js_obj(context, widget);

	if (pthread_create(&return_thread, NULL, (void*(*)(void*))widget_init, widget) != 0) {
		LOG_ERR("failed to start widget %s: %s", name, strerror(errno));
	}

	return return_thread;
}

void
join_widget_threads (struct wkline *wkline) {
	unsigned short i;
	if (widget_threads && (widgets_len > 0)) {
		eventfd_write(wkline->efd, 1);
		LOG_DEBUG("gracefully shutting down widget threads...");
		for (i = 0; i < widgets_len; i++) {
			if (widget_threads[i]) {
				/* this call may fail if the thread never
				   entered the main thread loop */
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

	if (get_config_option_boolean_silent(widget->config, "debug")) {
		LOG_DEBUG("updating widget %s: %s", widget->name, widget->data);
	}

	snprintf(script, script_length + 1, script_template, widget->name, widget->data);

	webkit_web_view_execute_script(widget->wkline->web_view, script);
	free(script);

	return FALSE; /* only run once */
}

bool
web_view_update (struct widget *widget, JSValueRef *args) {
	JSStringRef str_ondatachanged = JSStringCreateWithUTF8CString("onDataChanged");
	JSValueRef func = JSObjectGetProperty(widget->js_context, widget->js_object, str_ondatachanged, NULL);
	JSObjectRef function = JSValueToObject(widget->js_context, func, NULL);
	JSStringRelease(str_ondatachanged);

	if (!JSObjectIsFunction(widget->js_context, function)) {
		LOG_DEBUG("onDataChanged callback for '%s' widget is not a function or is not set", widget->name);

		return false;
	}

	JSObjectCallAsFunction(widget->js_context, function, NULL, LENGTH(args), args, NULL);

	return true;
}

void
window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, void *context, void *window_object, void *user_data) {
	LOG_DEBUG("webkit: window object cleared");
	struct wkline *wkline = user_data;

	json_t *widget;
	json_t *widgets = json_object_get(wkline->config, "widgets");
	size_t index;

	join_widget_threads(wkline);

	widgets_len = json_array_size(widgets);
	widget_threads = malloc(widgets_len * sizeof(pthread_t));

	LOG_DEBUG("starting %i widget threads", widgets_len);
	json_array_foreach(widgets, index, widget) {
		widget_threads[index] = spawn_widget(wkline,
		                                     context,
		                                     json_object_get(widget, "config"),
		                                     json_string_value(json_object_get(widget, "module")));
	}
}
