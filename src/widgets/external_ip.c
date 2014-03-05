#include "widgets.h"
#include "util/curl.h"
#include "src/config.h"
#include "external_ip.h"


/* externalIp.get_address method callback implementation */
static JSValueRef get_external_ip(JSContextRef context,
									JSObjectRef function,
									JSObjectRef thisObject,
									size_t argumentCount,
									const JSValueRef arguments[],
									JSValueRef *exception)
{
	const char *address;
	JSStringRef string;
	address = wkline_curl_request(wkline_widget_external_ip_address);
	string = JSStringCreateWithUTF8CString(address);
	return  JSValueMakeString(context, string);
}

/* Class method declarations */
static const JSStaticFunction external_ip_staticfuncs[] =
{
	{"get_address", get_external_ip, kJSPropertyAttributeReadOnly},
	{NULL, NULL, 0}
};

static const JSClassDefinition notification_def =
{
	0,						// version
	kJSClassAttributeNone,	// attributes
	"externalIp",			// className
	NULL,					// parentClass
	NULL,					// staticValues
	external_ip_staticfuncs,// staticFunctions
	NULL,					// initialize
	NULL,					// finalize
	NULL,					// hasProperty
	NULL,					// getProperty
	NULL,					// setProperty
	NULL,					// deleteProperty
	NULL,					// getPropertyNames
	NULL,					// callAsFunction
	NULL,					// callAsConstructor
	NULL,					// hasInstance  
	NULL					// convertToType
};

typedef struct wk_dimensions_t {
	int w;
	int h;
} wk_dimensions_t;

/* Callback - JavaScript window object has been cleared */
void add_class_external_ip(WebKitWebView  *web_view,
							WebKitWebFrame *frame,
							gpointer context,
							gpointer window_object,
							gpointer user_data)
{
	/* Add classes to JavaScriptCore */
	JSClassRef classDef = JSClassCreate(&notification_def);
	JSObjectRef classObj = JSObjectMake(context, classDef, context);
	JSObjectRef globalObj = JSContextGetGlobalObject(context);
	JSStringRef str = JSStringCreateWithUTF8CString("externalIp");
	JSObjectSetProperty(context, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
}
