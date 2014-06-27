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
append_flag (int fd, int flag) {
	int err;
	EINTRWRAP(err, fcntl(fd, F_GETFL, 0));
	assert(err >= 0);
	const int flags = err | flag;
	EINTRWRAP(err, fcntl(fd, F_SETFL, flags));
	assert(err >= 0);
}

static inline void
close_fd (int *fd) {
	if (*fd != -1) {
		int err;
		EINTRWRAP(err, close(*fd));
		*fd = -1;
	}
}

static inline int
read_fd (int *fd, struct Process *proc, WriteCallback cb) {
	char buf[BUFSIZ];
	int ret;
	EINTRWRAP(ret, read(*fd, buf, sizeof(buf)));
	if (!ret) {
		close_fd(fd);
	}
	else if (( ret > 0) && cb) {
		cb(proc, buf, ret);
	}

	return ret;
}

int
process (struct Process *proc) {
	assert(proc);
	assert(proc->path);
	assert(proc->argv);
	memset(proc->error, 0, sizeof(proc->error));
	int stdin[2], stdout[2], stderr[2], close_pipe[2];
	int err;
	EINTRWRAP(err, pipe(stdin));
	assert(!err);
	EINTRWRAP(err, pipe(stdout));
	assert(!err);
	EINTRWRAP(err, pipe(stderr));
	assert(!err);
	EINTRWRAP(err, pipe(close_pipe));
	assert(!err);

	append_flag(close_pipe[1], FD_CLOEXEC);

	const int pid = fork();

	if (pid == -1) {
		EINTRWRAP(err, close(close_pipe[1]));
		EINTRWRAP(err, close(close_pipe[0]));
		EINTRWRAP(err, close(stdin[1]));
		EINTRWRAP(err, close(stdin[0]));
		EINTRWRAP(err, close(stdout[1]));
		EINTRWRAP(err, close(stdout[0]));
		EINTRWRAP(err, close(stderr[1]));
		EINTRWRAP(err, close(stderr[0]));
		snprintf(proc->error, sizeof(proc->error), "Fork failed %d", errno);

		return -1;
	}
	else if (pid == 0) {
		EINTRWRAP(err, close(close_pipe[0]));
		EINTRWRAP(err, close(stdin[1]));
		EINTRWRAP(err, close(stdin[1]));
		EINTRWRAP(err, close(stdout[0]));
		EINTRWRAP(err, close(stderr[0]));

		EINTRWRAP(err, dup2(stdin[0], STDIN_FILENO));
		EINTRWRAP(err, close(stdin[0]));
		EINTRWRAP(err, dup2(stdout[1], STDOUT_FILENO));
		EINTRWRAP(err, close(stdout[1]));
		EINTRWRAP(err, dup2(stderr[1], STDERR_FILENO));
		EINTRWRAP(err, close(stderr[1]));

		if (proc->cwd && strlen(proc->cwd)) {
			EINTRWRAP(err, chdir(proc->cwd));
		}
		const int ret = execv(proc->path, (char*const*)proc->argv);

		/* notify the parent process */
		const char c = 'c';
		EINTRWRAP(err, write(close_pipe[1], &c, 1));
		EINTRWRAP(err, close(close_pipe[1]));
		_exit(1);
		(void)ret;
	}
	else {
		/* parent */
		EINTRWRAP(err, close(close_pipe[1]));
		EINTRWRAP(err, close(stdin[0]));
		EINTRWRAP(err, close(stdout[1]));
		EINTRWRAP(err, close(stderr[1]));

		/* printf("fork, in parent\n"); */

		append_flag(stdin[1], O_NONBLOCK);

		int status = 0, max, closed = 0;
		size_t stdin_written = 0;
		while (stdin_written < proc->stdin_length) {
			EINTRWRAP(err, write(stdin[1], proc->stdin_buffer + stdin_written, proc->stdin_length - stdin_written));
			if (err > 0) {
				stdin_written += err;
			}
			else if (err < 0) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
					break;
				}
				else {
					status = -1;
					goto cleanup;
				}
			}
		}
		assert(stdin_written <= proc->stdin_length);
		if (stdin_written == proc->stdin_length) {
			close_fd(&stdin[1]);
			++closed;
		}

		max = close_pipe[0];
		if (stdout[0] > max) {
			max = stdout[0];
		}
		if (stderr[0] > max) {
			max = stderr[0];
		}
		if (stdin[1] > max) {
			max = stdin[1];
		}

		do {
			fd_set r, w;
			FD_ZERO(&r);
			FD_ZERO(&w);
			if (close_pipe[0] != -1) {
				FD_SET(close_pipe[0], &r);
			}
			if (stdout[0] != -1) {
				FD_SET(stdout[0], &r);
			}
			if (stderr[0] != -1) {
				FD_SET(stderr[0], &r);
			}
			if (stdin[1] != -1) {
				FD_SET(stdin[1], &w);
			}

			const int ret = select(max + 1, &r, &w, 0, 0);
			assert(ret);
			if (ret > 0) {
				if ((stdout[0] != -1) && FD_ISSET(stdout[0], &r)) {
					const int ret = read_fd(&stdout[0], proc, proc->stdout_cb);
					if (!ret) {
						++closed;
					}
					else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s stdout %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}
				if ((stderr[0] != -1) && FD_ISSET(stderr[0], &r)) {
					const int ret = read_fd(&stderr[0], proc, proc->stderr_cb);
					if (!ret) {
						++closed;
					}
					else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s stderr %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}
				if ((close_pipe[0] != -1) && FD_ISSET(close_pipe[0], &r)) {
					const int ret = read_fd(&close_pipe[0], 0, 0);
					if (ret > 0) {
						snprintf(proc->error, sizeof(proc->error), "exec failed process %s %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
					else if (ret < 0) {
						snprintf(proc->error, sizeof(proc->error), "read failed for process %s close pipe %d", proc->path, errno);
						status = -1;
						goto cleanup;
					}
				}

				if ((stdin[1] != -1) && FD_ISSET(stdin[1], &w)) {
					while (stdin_written < proc->stdin_length) {
						EINTRWRAP(err, write(stdin[1], proc->stdin_buffer + stdin_written, proc->stdin_length - stdin_written));
						if (err > 0) {
							stdin_written += err;
						}
						else if (err < 0) {
							if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
								break;
							}
							else {
								status = -1;
								goto cleanup;
							}
						}
					}
					assert(stdin_written <= proc->stdin_length);
					if (stdin_written == proc->stdin_length) {
						close_fd(&stdin[1]);
						++closed;
					}
				}
			}
			else if (errno != EINTR) {
				snprintf(proc->error, sizeof(proc->error), "select failed %d", errno);
				status = -1;
				break;
			}
		} while (closed < 3);
cleanup:
		{
			int status_code;
			EINTRWRAP(err, waitpid(pid, &status_code, 0));
			if (status != -1) {
				status = status_code;
			}
			close_fd(&close_pipe[0]);
			close_fd(&stdin[1]);
			close_fd(&stdout[0]);
			close_fd(&stderr[0]);
		}

		return status;
	}
}
