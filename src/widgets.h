#ifndef WIDGETS_H
#define WIDGETS_H

#include <gmodule.h>
#include <JavaScriptCore/JavaScript.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/epoll.h>
#include <webkit/webkit.h>

#include "util/config.h"
#include "util/copy_prop.h"
#include "util/gdk_helpers.h"
#include "util/log.h"

struct js_callback_arg {
	JSType type;
	union {
		JSObjectRef object;
		bool boolean;
		char *null;
		const char *string;
		int number;
	} value;
};

struct js_callback_data {
	struct widget *widget;
	struct js_callback_arg *args;
	int args_len;
};

struct widget {
	char *name;
	json_t *config;
	char *data;
	struct wkline *wkline;
	JSContextRef js_context;
	JSObjectRef js_object;
	JSStaticFunction *js_staticfuncs;
};

typedef void (*widget_init_t)(void*);

pthread_mutex_t update_mutex;
pthread_cond_t update_cond;

void join_widget_threads ();
bool web_view_callback (struct js_callback_data *data);
void document_load_finished_cb (WebKitWebView *web_view, WebKitWebFrame *web_frame, void *user_data);
void window_object_cleared_cb (WebKitWebView *web_view, GParamSpec *pspec, void *context, void *window_object, void *user_data);

#define widget_init_config_string(WIDGET, KEY, TARGET) \
	{ json_t *CONF = get_config_option(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_string_value(CONF); } }
#define widget_init_config_integer(WIDGET, KEY, TARGET) \
	{ json_t *CONF = get_config_option(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_integer_value(CONF); } }
#define widget_init_config_real(WIDGET, KEY, TARGET) \
	{ json_t *CONF = get_config_option(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_real_value(CONF); } }
#define widget_init_config_boolean(WIDGET, KEY, TARGET) \
	{ json_t *CONF = get_config_option(WIDGET, KEY, true); \
	  if (CONF != NULL) { TARGET = json_is_true(CONF); } }

#define widget_data_callback(WIDGET, ...) \
	{ struct js_callback_arg args[] = { __VA_ARGS__ }; \
	  struct js_callback_data data = { .widget = WIDGET, .args = args, .args_len = LENGTH(args) }; \
	  pthread_mutex_lock(&update_mutex); \
	  g_idle_add((GSourceFunc)web_view_callback, &data); \
	  pthread_cond_wait(&update_cond, &update_mutex); \
	  pthread_mutex_unlock(&update_mutex); }
#define widget_data_arg_boolean(ARG) { .type = kJSTypeBoolean, .value.boolean = ARG }
#define widget_data_arg_null()       { .type = kJSTypeNull, .value.null = NULL }
#define widget_data_arg_number(ARG)  { .type = kJSTypeNumber, .value.number = ARG }
#define widget_data_arg_object(ARG)  { .type = kJSTypeObject, .value.object = ARG }
#define widget_data_arg_string(ARG)  { .type = kJSTypeString, .value.string = ARG }
#define widget_data_arg_undefined()  { .type = kJSTypeUndefined, .value.null = NULL }

#define MAX_EVENTS 10
#define widget_epoll_init(WIDGET) \
	int efd, nfds; \
	struct epoll_event event, events[MAX_EVENTS]; \
	if ((efd = epoll_create1(0)) == -1) { LOG_ERR("failed to create epoll instance: %s", strerror(errno)); return 0; } \
	event.data.fd = WIDGET->wkline->efd; event.events = EPOLLIN | EPOLLET; \
	if (epoll_ctl(efd, EPOLL_CTL_ADD, WIDGET->wkline->efd, &event) == -1) { LOG_ERR("failed to add fd to epoll instance: %s", strerror(errno)); return 0; }
#define widget_epoll_wait_goto(WIDGET, TIMEOUT_SECONDS, GOTO_LABEL) nfds = epoll_wait(efd, events, MAX_EVENTS, TIMEOUT_SECONDS * 1000); \
	if (nfds && (events[0].data.fd == WIDGET->wkline->efd)) { goto GOTO_LABEL; }

#endif
