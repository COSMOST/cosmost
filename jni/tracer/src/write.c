#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_write_fd = -1;
static __thread off64_t last_write_off = -1;
static __thread size_t last_write_size = 0;
static __thread char write_iostr[IOSTR_BUFSIZE];
#endif

static __thread char write_path[PATH_MAX];

static write_t _write;

static int write_init(void)
{
	if (!_write) {
		_write = (write_t) dlsym(RTLD_NEXT, "write");
		if (!_write)
    		_write = (write_t) dlsym(RTLD_NEXT, "__write");
	}

	if (!_write) {
		errno = EIO;
		ALOGE("NULL _write!");
		return -1;
	}

	return 0;
}

void write_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	/* file closing or thwrite exiting */
	if ((fd == -1 && last_write_fd != -1) ||
		(fd != -1 && fd == last_write_fd)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"write\",\"Path\":\"%s\"", write_path);
#endif

		info("\"op\":\"%s\",\"last_write_fd\":%d,\"last_write_size\":%llu,"
			"\"last_write_off\":%lld",
			fd == -1 ? "write (exit)" : "write (close)",
			last_write_fd, (uint64_t)last_write_size, (int64_t)last_write_off);

		notify_read_write(fd == -1 ? "write (exit)" : "write (close)",
							write_path, last_write_fd, last_write_size,
							last_write_off, write_iostr);

		last_write_fd = -1;
	}
#endif
}

ssize_t write(int fd, const void *buf, size_t size)
{
	if (write_init() == -1)
		return -1;

	/* actual syscall */
	ssize_t ret = _write(fd, buf, size);
#ifdef TEST_OPS
	if (ret > 0 && fd_to_path(fd, write_path) != -1 &&
		strncmp(write_path, "/sys/", strlen("/sys/")) &&
		strncmp(write_path, "/proc/", strlen("/proc/")) &&
		strncmp(write_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(write_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"write\",\"Path\":\"%s\"", write_path);
#endif

	if (ret > 0 && _notify) {
#ifdef TEST_OPS
		if (strncmp(write_path, "/sys/", strlen("/sys/")) &&
			strncmp(write_path, "/proc/", strlen("/proc/")) &&
			strncmp(write_path, "pipe:[", strlen("pipe:[")) &&
			strncmp(write_path, "/dev/", strlen("/dev/")))
			test("\"Stage\":2,\"Op\":\"write\",\"Path\":\"%s\"", write_path);
#endif

#ifdef PER_FILE_BATCHING
		if (last_write_fd == fd) {
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, write_path);
			else if (!batch_read_write_first("write", write_path, &last_write_fd, &last_write_size,
											&last_write_off, fd, ret, off, write_iostr)) {
				return ret;
			}
		}
#endif

		/* different file, dispatch batched write notifications */
		if (fd_to_path(fd, write_path) != -1 && notify(fd, S_IFREG, write_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"write\",\"Path\":\"%s\"", write_path);
#endif
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, write_path);
			else {
#ifdef PER_FILE_BATCHING
				batch_read_write_second("write", write_path, &last_write_fd, &last_write_size,
											&last_write_off, fd, ret, off, write_iostr);
#else
				notify_read_write("write", write_path, fd, size, off, NULL);
#endif
			}
		}
	}

	return ret;
}

