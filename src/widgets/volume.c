#include "widgets.h"
#include "volume.h"
#include "src/config.h"

typedef struct {
	JSContextRef context;
	JSObjectRef object;
} RefContextObject;


struct volume_data{
	snd_mixer_elem_t *elem;
	snd_mixer_t *mixer;
	RefContextObject *ref;
	int volume;
};

static void call_on_change(RefContextObject *ref) {
	/* Get onChange property */
	JSStringRef string_onchange;
	string_onchange = JSStringCreateWithUTF8CString("onChange");
	JSValueRef func = JSObjectGetProperty(ref->context, ref->object, string_onchange, NULL);
	JSObjectRef function = JSValueToObject(ref->context, func, NULL);
	JSStringRelease(string_onchange);
	
	if (!JSObjectIsFunction(ref->context, function)) {
		g_message("JSObject is not function or is not set");
		return;
	}

	JSValueRef result = JSObjectCallAsFunction(ref->context, // The execution context to use
											function, // The JSObject to call as a function.
											ref->object, // The object to use as "this," or NULL to use the global object as "this."
											0, //  An integer count of the number of arguments in arguments.
											NULL, // A JSValue array of arguments to pass to the function. Pass NULL if argumentCount is 0.
											NULL); // A pointer to a JSValueRef in which to store an exception, if any. Pass NULL if you do not care to store an exception.
}


void
thread_wait_mixer_change(void *user_data){
	struct volume_data *widget = user_data;
	snd_mixer_selem_id_t *sid;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err, wait_err,  muted;
	unsigned short revents;
	long volume_min, volume_max, volume;

	while(1){
		// Code mostly from the alsamixer main loop
		n = 1 + snd_mixer_poll_descriptors_count(widget->mixer);
		if (n != nfds) {
			free(pollfds);
			nfds = n;
			pollfds = calloc(nfds, sizeof *pollfds);
			pollfds[0].fd = fileno(stdin);
			pollfds[0].events = POLLIN;
		}
		err = snd_mixer_poll_descriptors(widget->mixer, &pollfds[1], nfds - 1);
		if (err < 0) {
			wklog("alsa: can't get poll descriptors: %i", err);
			break;
		}
		wait_err = snd_mixer_wait(widget->mixer, -1);
		if (wait_err < 0) {
			wklog("alsa: wait error");
		}
		n = poll(pollfds, nfds, -1);
		if (n < 0) {
			if (errno == EINTR) {
				pollfds[0].revents = 0;
			}
			else {
				wklog("alsa: poll error");
				break;
			}
		}
		if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		if (n > 0) {
			err = snd_mixer_poll_descriptors_revents(widget->mixer, &pollfds[1], nfds - 1, &revents);
			if (err < 0) {
				wklog("alsa: fatal error: %i", err);
				break;
			}
			if (revents & (POLLERR | POLLNVAL)){
				wklog("alsa: closing mixer");
				break;
			}
			else if (revents & POLLIN) {
				snd_mixer_handle_events(widget->mixer);
			}
		}
		snd_mixer_selem_get_playback_volume_range(widget->elem, &volume_min, &volume_max);
		snd_mixer_selem_get_playback_volume(widget->elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
		snd_mixer_selem_get_playback_switch(widget->elem, SND_MIXER_SCHN_FRONT_LEFT , &muted);
		volume *= muted; /* if muted set volume to 0 */
		widget->volume = 100 * (volume - volume_min) / (volume_max - volume_min);
		call_on_change(widget->ref);
	}
}

/* Class initialize */
static void widget_init_cb(JSContextRef ctx,
                            JSObjectRef object)
{
	struct volume_data *widget = JSObjectGetPrivate(object);
	snd_mixer_selem_id_t *sid;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err, wait_err;
	unsigned short revents;

	widget->ref = g_new(RefContextObject, 1);
	widget->ref->context = ctx;
	widget->ref->object = object;

	// open mixer
	snd_mixer_open(&widget->mixer, 0);
	snd_mixer_attach(widget->mixer, wkline_widget_volume_card);
	snd_mixer_selem_register(widget->mixer, NULL, NULL);
	snd_mixer_load(widget->mixer);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, wkline_widget_volume_selem);
	widget->elem = snd_mixer_find_selem(widget->mixer, sid);

	g_thread_new("widget", (GThreadFunc)thread_wait_mixer_change, widget);
}

/* Class finalize */
static void widget_destroy_cb(JSObjectRef object)
{
	struct volume_data *widget = JSObjectGetPrivate(object);
	//free(pollfds);
	//snd_mixer_close(widget->mixer);
	//free(widget);
}

static JSValueRef get_volume(JSContextRef context,
									JSObjectRef function,
									JSObjectRef thisObject,
									size_t argumentCount,
									const JSValueRef arguments[],
									JSValueRef *exception)
{
	struct volume_data *widget = JSObjectGetPrivate(thisObject);
	printf("%d\n", widget->volume);
	return JSValueMakeNumber(context, widget->volume);
}

/* Class constructor */
static JSObjectRef widget_constructor_cb(JSContextRef context,
									JSObjectRef constructor,
									size_t argumentCount,
									const JSValueRef arguments[],
									JSValueRef *exception)
{
    return constructor;
}

/* Class method declarations */
static const JSStaticFunction widget_staticfuncs[] =
{
	{"get_volume", get_volume, kJSPropertyAttributeReadOnly},
	{NULL, NULL, 0}
};

static const JSClassDefinition notification_def =
{
	0,						// version
	kJSClassAttributeNone,	// attributes
	"Volume",				// className
	NULL,					// parentClass
	NULL,					// staticValues
	widget_staticfuncs,		// staticFunctions
	widget_init_cb,			// initialize
	widget_destroy_cb,		// finalize
	NULL,					// hasProperty
	NULL,					// getProperty
	NULL,					// setProperty
	NULL,					// deleteProperty
	NULL,					// getPropertyNames
	NULL,					// callAsFunction
	widget_constructor_cb,	// callAsConstructor
	NULL,					// hasInstance  
	NULL					// convertToType
};

/* Callback - JavaScript window object has been cleared */
void add_class_volume(WebKitWebView  *web_view,
							WebKitWebFrame *frame,
							gpointer context,
							gpointer window_object,
							gpointer user_data)
{
	struct volume_data *widget;
	widget = calloc(0, sizeof widget);
	/* Add classes to JavaScriptCore */
	JSClassRef classDef = JSClassCreate(&notification_def);
	JSObjectRef classObj = JSObjectMake(context, classDef, widget);
	JSObjectRef globalObj = JSContextGetGlobalObject(context);
	JSStringRef str = JSStringCreateWithUTF8CString("Volume");
	JSObjectSetProperty(context, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
}
