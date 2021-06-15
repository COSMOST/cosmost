#define LOG_TAG "Storage-IO-UIUC"

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <sys/types.h>

#include "ops.h"
#include "process.h"
#include "fs.h"
#include "counter.h"

#define MAX_PREFIX_SIZE 512

static ssize_t init_buffer(char *buf)
{
	static __thread pid_t thread_pid;

	static pid_t procid;
	static ssize_t prefix_len;
	static char prefix[MAX_PREFIX_SIZE];

	if (!buf) {
		err("\"Buffer\":0");
		return -1;
	}

	/* check if this thread needs (re)initialization (due to fork) */
	pid_t new_pid = getpid();
	if (thread_pid == new_pid)
		return prefix_len;

	/* mark for (re)initialization */
	buf[0] = '\0';

	/* check if process-wide (re)initialization is needed */
	pid_t old_pid;
	while (1) {

		old_pid = __atomic_load_n (&procid, __ATOMIC_SEQ_CST);
		if (old_pid == new_pid)
			break; // not needed

		/* perform process-wide (re)initialization */
		prefix_len = snprintf(prefix, MAX_PREFIX_SIZE,
						"{\"PID\":%d,\"PROC\":\"%s\",\"CNTR\":%llu,\"REC\":",
						new_pid, process_name(), cntr());

		if (prefix_len < 0) {
			err("\"Err\":\"%s\"", strerror(errno));
			return -1;
		}

		if (prefix_len >= MAX_PREFIX_SIZE) {
			err("\"Prefix too long\":%d", prefix_len);
			return -1;
		}

#ifdef WRITE_TO_FILE
		if (thread_fd != -1) {
			_close(thread_fd);
			thread_fd = -1;
		}
#endif
		if (__sync_bool_compare_and_swap(&procid, old_pid, new_pid))
			break;
	}

	/* perform thread (re)initialization */
	if (!buf[0]) {
		thread_pid = procid;
		strcpy(buf, prefix);
	}

	return prefix_len;
}

int blog(const char* format, ...)
{
	static __thread char buffer[BUFFER_SIZE];
 
	ssize_t ret = init_buffer(buffer);
	if (ret < 0)
		return -1;

	size_t size = ret;

	info("\"Buffer\":%p,\"Size\":%d,\"Strlen(buffer)\":%d",
			buffer, size, strlen(buffer));

	va_list args;
	va_start(args, format);
	ret = vsnprintf(&buffer[size], (BUFFER_SIZE - size - 2), format, args);
	va_end(args);

	info("\"Buffer\":%p,\"Size\":%d,\"Strlen(buffer)\":%d",
			buffer, size, strlen(buffer));

	size += ret;
	if (size >= (BUFFER_SIZE - 2)) {
		err("\"Data too long\":%d", size);
		return -1;
	}

	/* JSON formatting */
	buffer[size] = '}';
	size++;
	buffer[size] = '\0';

	info("\"Buffer\":%p,\"Size\":%d,\"Strlen(buffer)\":%d",
			buffer, size, strlen(buffer));

	return __android_log_write(ANDROID_LOG_INFO, LOG_TAG, buffer);
}
