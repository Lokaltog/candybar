#include "widgets.h"
#include "email_imap.h"
#include "util/process.h"
#include <ctype.h>
#include <assert.h>

struct Buffer {
	char *stdOutBuffer, *stdErrBuffer;
	size_t stdOutUsed, stdOutLen, stdErrUsed, stdErrLen;
};

static inline void freeBuffer(struct Buffer *buffer)
{
	if (buffer->stdOutBuffer) {
		free(buffer->stdOutBuffer);
		buffer->stdOutBuffer = 0;
		buffer->stdOutUsed = buffer->stdOutLen = 0;
	}
	if (buffer->stdErrBuffer) {
		free(buffer->stdErrBuffer);
		buffer->stdErrBuffer = 0;
		buffer->stdErrUsed = buffer->stdErrLen = 0;
	}
}

static inline void freeProcess(struct Process *proc)
{
	for (int i=0; proc->argv[i]; ++i)
		free(proc->argv[i]);
	free(proc->argv);
}

static inline void
writeBuffer(char **buf, size_t *used, size_t *bufLen, const char *data, size_t dataLen)
{
	if (!*buf) {
		*buf = calloc(1024, 1);
		*bufLen = 1024;
	}
	while (*used + dataLen >= *bufLen) {
		size_t add = *bufLen * 1.5;
		if (add < dataLen) {
			add = dataLen;
		}
		*bufLen += add;
		*buf = realloc(*buf, *bufLen);
	}
	memcpy(*buf + *used, data, dataLen);
	*used += dataLen;
	(*buf)[*used] = 0;
	assert(*used <= *bufLen);
}

void
writeStdOut(struct Process *proc, const char *data, size_t len)
{
	struct Buffer *buf = (struct Buffer*)proc->userData;
	writeBuffer(&buf->stdOutBuffer, &buf->stdOutUsed, &buf->stdOutLen, data, len);
}

void
writeStdErr(struct Process *proc, const char *data, size_t len)
{
	struct Buffer *buf = (struct Buffer*)proc->userData;
	writeBuffer(&buf->stdErrBuffer, &buf->stdErrUsed, &buf->stdErrLen, data, len);
}

static int
widget_update (struct widget *widget, struct widget_config config) {
	CURL *curl;
	CURLcode status;
	char *data;
	long code;

	/* connect and handle IMAP responses */
	curl = curl_easy_init();
	data = malloc(CURL_BUF_SIZE);
	if (!curl || !data) {
		return 0;
	}

	write_result_t write_result = {
		.data = data,
		.pos = 0
	};

	curl_easy_setopt(curl, CURLOPT_USERNAME, config.username);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password);
	curl_easy_setopt(curl, CURLOPT_URL, config.address);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "SEARCH UNSEEN");

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, candybar_curl_write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	if (config.ssl_verify) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	status = curl_easy_perform(curl);
	if (status != CURLE_OK) {
		LOG_ERR("unable to request data from %s (this error may be temporary): %s",
				config.address,
				curl_easy_strerror(status));

		return 0;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

	if (code != 0) {
		LOG_ERR("server responded with code %ld", code);

		return 0;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	data[write_result.pos] = '\0';

	/* count unread message IDs */

	/* the server responds with a string like "* SEARCH 1 2 3 4" where the
	   numbers are message IDs */
	char *str, *saveptr, *delim = " *";
	int unread = -1;

	str = strtok_r(data, delim, &saveptr);
	while (str != NULL) {
		str = strtok_r(NULL, delim, &saveptr);
		unread++;
	}

	free(data);

	widget_data_callback(widget,
						 widget_data_arg_number(unread),
						 widget_data_arg_string(config.username));

	return 0;
}

static const char *next(const char *ch, int space)
{
	while (*ch && !isspace(*ch) == !space)
		++ch;
	return ch;
}

static int parseCommandLineArguments(const char *passwordCommand, struct Process *proc)
{
	const char *start = next(passwordCommand, 1);
	if (!*start)
		return -1;
	const char *end = next(start, 0);
	if (!*end) {
		proc->path = strdup(start);
	} else {
		proc->path = strndup(start, end - start);
	}
	proc->argv = calloc(1024, sizeof(char*));
	proc->argv[0] = proc->path;
	int argvIdx = 1;

	while ((start = next(end, 1))) {
		end = next(start, 0);
		if (!*end) {
			proc->argv[argvIdx++] = strdup(start);
			break;
		} else {
			proc->argv[argvIdx++] = strndup(start, end - start);
		}
	}
	return 1;
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "address", config.address);
	widget_init_config_string(widget->config, "username", config.username);
	widget_init_config_string(widget->config, "password", config.password);
	widget_init_config_string(widget->config, "passwordcommand", config.passwordcommand);
	widget_init_config_boolean(widget->config, "ssl_verify", config.ssl_verify);
	widget_init_config_integer(widget->config, "refresh_interval", config.refresh_interval);

	if (!config.username) {
		LOG_INFO("email_imap: username not set, disabling widget");

		return 0;
	}

	struct Buffer buffer;
	memset(&buffer, 0, sizeof(buffer));
	if (strlen(config.passwordcommand)) {
		struct Process proc;
		memset(&proc, 0, sizeof(proc));
		proc.stdErrCb = writeStdErr;
		proc.stdOutCb = writeStdOut;
		proc.userData = &buffer;
		if (parseCommandLineArguments(config.passwordcommand, &proc) == -1) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "Can't parse passwordcommand: \"%s\"", config.passwordcommand);
			LOG_ERR(buf);
			return 0;
		}
		proc.argv = malloc(sizeof(char*) * 2);
		proc.argv[0] = proc.path;
		proc.argv[1] = 0;
		const int ret = process(&proc);
		freeProcess(&proc);
		if (ret != 0) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "Process error: %s => %d: %s/%s\n", proc.path, ret, proc.error, buffer.stdErrBuffer);
			LOG_ERR(buf);
			return 0;
		}
		config.password = buffer.stdOutBuffer;
	}

	widget_epoll_init(widget);
	while (true) {
		widget_update(widget, config);
		widget_epoll_wait_goto(widget, config.refresh_interval, cleanup);
	}

cleanup:

	widget_epoll_cleanup(widget);
	widget_clean_exit(widget);
	freeBuffer(&buffer);
}
