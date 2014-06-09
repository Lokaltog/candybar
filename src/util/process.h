#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

struct Process;
typedef void (*WriteCallback)(struct Process *proc, const char *, size_t);

struct Process {
    WriteCallback stdErrCb, stdOutCb;
    char *stdInBuffer;
    size_t stdInLength;
    char error[1024];
    void *userData;

    char *path;
    char *cwd;
    char **argv;
};

int process(struct Process *proc);
#endif
