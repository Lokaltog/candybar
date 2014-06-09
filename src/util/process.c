#include "process.h"
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EINTRWRAP(VAR, BLOCK)                   \
	do {                                        \
		VAR = BLOCK;                            \
	} while (VAR == -1 && errno == EINTR)

static inline void
appendFlag(int fd, int flag)
{
	int err;
	EINTRWRAP(err, fcntl(fd, F_GETFL, 0));
	assert(err >= 0);
	const int flags = err | flag;
	EINTRWRAP(err, fcntl(fd, F_SETFL, flags));
	assert(err >= 0);
}

static inline void
closeFd(int *fd)
{
	if (*fd != -1) {
		int err;
		EINTRWRAP(err, close(*fd));
		*fd = -1;
	}
}

static inline int
readFd(int *fd, struct Process *proc, WriteCallback cb)
{
	char buf[BUFSIZ];
	int ret;
	EINTRWRAP(ret, read(*fd, buf, sizeof(buf)));
	if (!ret) {
		closeFd(fd);
	} else if (ret > 0 && cb) {
		cb(proc, buf, ret);
	}
	return ret;
}

int
process(struct Process *proc)
{
	assert(proc);
	assert(proc->path);
	assert(proc->argv);
	memset(proc->error, 0, sizeof(proc->error));
	int stdIn[2], stdOut[2], stdErr[2], closePipe[2];
	int err;
	EINTRWRAP(err, pipe(stdIn));
	assert(!err);
	EINTRWRAP(err, pipe(stdOut));
	assert(!err);
	EINTRWRAP(err, pipe(stdErr));
	assert(!err);
	EINTRWRAP(err, pipe(closePipe));
	assert(!err);

	appendFlag(closePipe[1], FD_CLOEXEC);

	const int pid = fork();

	if (pid == -1) {
		EINTRWRAP(err, close(closePipe[1]));
		EINTRWRAP(err, close(closePipe[0]));
		EINTRWRAP(err, close(stdIn[1]));
		EINTRWRAP(err, close(stdIn[0]));
		EINTRWRAP(err, close(stdOut[1]));
		EINTRWRAP(err, close(stdOut[0]));
		EINTRWRAP(err, close(stdErr[1]));
		EINTRWRAP(err, close(stdErr[0]));
		snprintf(proc->error, sizeof(proc->error), "Fork failed %d", errno);
		return -1;
	} else if (pid == 0) {
		EINTRWRAP(err, close(closePipe[0]));
		EINTRWRAP(err, close(stdIn[1]));
		EINTRWRAP(err, close(stdIn[1]));
		EINTRWRAP(err, close(stdOut[0]));
		EINTRWRAP(err, close(stdErr[0]));

		EINTRWRAP(err, dup2(stdIn[0], STDIN_FILENO));
		EINTRWRAP(err, close(stdIn[0]));
		EINTRWRAP(err, dup2(stdOut[1], STDOUT_FILENO));
		EINTRWRAP(err, close(stdOut[1]));
		EINTRWRAP(err, dup2(stdErr[1], STDERR_FILENO));
		EINTRWRAP(err, close(stdErr[1]));

		if (proc->cwd && strlen(proc->cwd))
			EINTRWRAP(err, chdir(proc->cwd));
		const int ret = execv(proc->path, (char *const *)proc->argv);

		// notify the parent process
		const char c = 'c';
		EINTRWRAP(err, write(closePipe[1], &c, 1));
		EINTRWRAP(err, close(closePipe[1]));
		_exit(1);
		(void)ret;
	} else {
		// parent
		EINTRWRAP(err, close(closePipe[1]));
		EINTRWRAP(err, close(stdIn[0]));
		EINTRWRAP(err, close(stdOut[1]));
		EINTRWRAP(err, close(stdErr[1]));

		//printf("fork, in parent\n");

		appendFlag(stdIn[1], O_NONBLOCK);

		int status = 0, max, closed = 0;
		size_t stdInWritten = 0;
		while (stdInWritten < proc->stdInLength) {
			EINTRWRAP(err, write(stdIn[1], proc->stdInBuffer + stdInWritten, proc->stdInLength - stdInWritten));
			if (err > 0) {
				stdInWritten += err;
			} else if (err < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				} else {
					status = -1;
					goto cleanup;
				}
			}
		}
		assert(stdInWritten <= proc->stdInLength);
		if (stdInWritten == proc->stdInLength) {
			closeFd(&stdIn[1]);
			++closed;
		}

		max = closePipe[0];
		if (stdOut[0] > max)
			max = stdOut[0];
		if (stdErr[0] > max)
			max = stdErr[0];
		if (stdIn[1] > max)
			max = stdIn[1];

		do {
			fd_set r, w;
			FD_ZERO(&r);
			FD_ZERO(&w);
			if (closePipe[0] != -1)
				FD_SET(closePipe[0], &r);
			if (stdOut[0] != -1)
				FD_SET(stdOut[0], &r);
			if (stdErr[0] != -1)
				FD_SET(stdErr[0], &r);
			if (stdIn[1] != -1)
				FD_SET(stdIn[1], &w);

			const int ret = select(max + 1, &r, &w, 0, 0);
			assert(ret);
			if (ret > 0) {
				if (stdOut[0] != -1 && FD_ISSET(stdOut[0], &r)) {
					const int ret = readFd(&stdOut[0], proc, proc->stdOutCb);
					if (!ret) {
						++closed;
					} else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s stdout %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}
				if (stdErr[0] != -1 && FD_ISSET(stdErr[0], &r)) {
					const int ret = readFd(&stdErr[0], proc, proc->stdErrCb);
					if (!ret) {
						++closed;
					} else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s stderr %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}
				if (closePipe[0] != -1 && FD_ISSET(closePipe[0], &r)) {
					const int ret = readFd(&closePipe[0], 0, 0);
					if (ret > 0) {
						snprintf(proc->error, sizeof(proc->error), "exec failed process %s %d", proc->path, errno);
						status = -1;
						goto cleanup;
					} else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s close pipe %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}

				if (stdIn[1] != -1 && FD_ISSET(stdIn[1], &w)) {
					while (stdInWritten < proc->stdInLength) {
						EINTRWRAP(err, write(stdIn[1], proc->stdInBuffer + stdInWritten, proc->stdInLength - stdInWritten));
						if (err > 0) {
							stdInWritten += err;
						} else if (err < 0) {
							if (errno == EAGAIN || errno == EWOULDBLOCK) {
								break;
							} else {
								status = -1;
								goto cleanup;
							}
						}
					}
					assert(stdInWritten <= proc->stdInLength);
					if (stdInWritten == proc->stdInLength) {
						closeFd(&stdIn[1]);
						++closed;
					}
				}

			} else if (errno != EINTR) {
				snprintf(proc->error, sizeof(proc->error), "select failed %d", errno);
				status = -1;
				break;
			}
		} while (closed < 3);
  cleanup:
		{
			int statusCode;
			EINTRWRAP(err, waitpid(pid, &statusCode, 0));
			if (status != -1)
				status = statusCode;
			closeFd(&closePipe[0]);
			closeFd(&stdIn[1]);
			closeFd(&stdOut[0]);
			closeFd(&stdErr[0]);
		}
		return status;
	}
}

