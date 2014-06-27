#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

struct Process;
typedef void (*WriteCallback)(struct Process *proc, const char*, size_t);

struct Process {
	WriteCallback stderr_cb, stdout_cb;
	char *stdin_buffer;
	size_t stdin_length;
	char error[1024];
	void *user_data;

	char *path;
	char *cwd;
	char **argv;
};

int process (struct Process *proc);

#endif
