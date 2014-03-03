#include "widgets.h"
#include "src/config.h"
#include "volume.h"

struct volume_data
{
	snd_mixer_elem_t* elem;
	snd_mixer_t *mixer;
	JSContextRef *ctx;
	JSObjectRef *object;
};

/* Class initialize */
static void widget_init_cb(JSContextRef ctx,
                            JSObjectRef object)
{
	struct volume_data *widget = JSObjectGetPrivate(object);
	snd_mixer_selem_id_t *sid;
	struct pollfd *pollfds = NULL;
	int nfds = 0, n, err, wait_err;
	unsigned short revents;

	// open mixer
	snd_mixer_open(&widget->mixer, 0);
	snd_mixer_attach(widget->mixer, wkline_widget_volume_card);
	snd_mixer_selem_register(widget->mixer, NULL, NULL);
	snd_mixer_load(widget->mixer);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, wkline_widget_volume_selem);
	widget->elem = snd_mixer_find_selem(widget->mixer, sid);
}

/* Class finalize */
static void widget_destroy_cb(JSObjectRef object)
{
	struct volume_data *widget = JSObjectGetPrivate(object);
	//free(pollfds);
	//snd_mixer_close(widget->mixer);
	free(widget);
}

static JSValueRef get_volume(JSContextRef context,
									JSObjectRef function,
									JSObjectRef thisObject,
									size_t argumentCount,
									const JSValueRef arguments[],
									JSValueRef *exception)
{
	struct volume_data *widget = JSObjectGetPrivate(thisObject);
	long volume_min, volume_max, volume;
	int muted;

	snd_mixer_selem_get_playback_volume_range(widget->elem, &volume_min, &volume_max);
	snd_mixer_selem_get_playback_volume(widget->elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);
	snd_mixer_selem_get_playback_switch(widget->elem, SND_MIXER_SCHN_FRONT_LEFT , &muted);
	volume *= muted; /* if muted set volume to 0 */
	return JSValueMakeNumber(context, 100 * (volume - volume_min) / (volume_max - volume_min));
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