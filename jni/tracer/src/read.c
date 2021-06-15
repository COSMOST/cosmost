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
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static __thread char read_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_read_fd = -1;
static __thread off64_t last_read_off = -1;
static __thread size_t last_read_size = 0;
static __thread char read_iostr[IOSTR_BUFSIZE];
#endif

static read_t _read;
static int read_init(void)
{
	if (!_read) {
		_read = (read_t) dlsym(RTLD_NEXT, "read");
		if (!_read)
    		_read = (read_t) dlsym(RTLD_NEXT, "__read");
	}

	if (!_read) {
		errno = EIO;
		err("\"_read\":0");
		return -1;
	}

	return 0;
}

void read_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	/* file closing or thread exiting */
	if ((fd == -1 && last_read_fd != -1) ||
		(fd != -1 && fd == last_read_fd)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"read\",\"Path\":\"%s\"", read_path);
#endif

		info("\"op\":\"%s\",\"last_read_fd\":%d,\"last_read_size\":%llu,"
			"\"last_read_off\":%lld",
			fd == -1 ? "read (exit)" : "read (close)",
			last_read_fd, (uint64_t)last_read_size, (int64_t)last_read_off);

		notify_read_write(fd == -1 ? "read (exit)" : "read (close)",
							read_path, last_read_fd, last_read_size,
							last_read_off, read_iostr);

		last_read_fd = -1;
	}
#endif
}

ssize_t read(int fd, void *buf, size_t size)
{
	if (read_init() == -1)
		return -1;

	/* actual syscall */
	ssize_t ret = _read(fd, buf, size);

#ifdef TEST_OPS
	if (ret > 0 && fd_to_path(fd, read_path) != -1 &&
		strncmp(read_path, "/sys/", strlen("/sys/")) &&
		strncmp(read_path, "/proc/", strlen("/proc/")) &&
		strncmp(read_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(read_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"read\",\"Path\":\"%s\"", read_path);
#endif

	if (ret > 0 && _notify) {

#ifdef TEST_OPS
	if (strncmp(read_path, "/sys/", strlen("/sys/")) &&
		strncmp(read_path, "/proc/", strlen("/proc/")) &&
		strncmp(read_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(read_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":2,\"Op\":\"read\",\"Path\":\"%s\"", read_path);
#endif

#ifdef PER_FILE_BATCHING
		if (last_read_fd == fd) {
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, read_path);
			else if (!batch_read_write_first("read", read_path, &last_read_fd, &last_read_size,
											&last_read_off, fd, ret, off, read_iostr)) {
				return ret;
			}
		}
#endif

		/* different file, dispatch batched read notifications */
		if (fd_to_path(fd, read_path) != -1 && notify(fd, S_IFREG, read_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"read\",\"Path\":\"%s\"", read_path);
#endif
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, read_path);
			else {
#ifdef PER_FILE_BATCHING
				batch_read_write_second("read", read_path, &last_read_fd, &last_read_size,
										&last_read_off, fd, ret, off, read_iostr);
#else
				notify_read_write("read", read_path, fd, size, off, NULL);
#endif
			}
		}
	}

	return ret;
}

