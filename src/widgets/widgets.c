#include "widgets.h"
#include "external_ip.h"
#include "volume.h"

/* Callback - JavaScript window object has been cleared */
void
window_object_cleared_cb(WebKitWebView  *web_view,
							WebKitWebFrame *frame,
							gpointer context,
							gpointer window_object,
							gpointer user_data){
	add_class_volume(web_view,frame,context,window_object,user_data);
	add_class_external_ip(web_view,frame,context,window_object,user_data);
}