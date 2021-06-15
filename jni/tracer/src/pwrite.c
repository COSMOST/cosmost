#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static __thread char pwrite_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_pwrite64_fd = -1;
static __thread off64_t last_pwrite64_off = -1;
static __thread size_t last_pwrite64_size = 0;
static __thread char pwrite64_iostr[IOSTR_BUFSIZE];
#endif

static pwrite64_t _pwrite64;

static int pwrite64_init(void)
{
	if (!_pwrite64) {
		_pwrite64 = (pwrite64_t) dlsym(RTLD_NEXT, "pwrite64");
		if (!_pwrite64)
    		_pwrite64 = (pwrite64_t) dlsym(RTLD_NEXT, "__pwrite64");
	}

	if (!_pwrite64) {
		errno = EIO;
		err("\"_pwrite64\":0");
		return -1;
	}

	return 0;
}

void pwrite64_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	/* file closing or thread exiting */
	if ((fd == -1 && last_pwrite64_fd != -1) ||
		(fd != -1 && fd == last_pwrite64_fd)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"pwrite64\",\"Path\":\"%s\"", pwrite_path);
#endif

		info("\"op\":\"%s\",\"last_pwrite64_fd\":%d,\"last_pwrite64_size\":%llu,"
			"\"last_pwrite64_off\":%lld",
			fd == -1 ? "pwrite64 (exit)" : "pwrite64 (close)",
			last_pwrite64_fd, (uint64_t)last_pwrite64_size, (int64_t)last_pwrite64_off);

		notify_read_write(fd == -1 ? "pwrite64 (exit)" : "pwrite64 (close)",
							pwrite_path, last_pwrite64_fd, last_pwrite64_size,
							last_pwrite64_off, pwrite64_iostr);

		last_pwrite64_fd = -1;
	}
#endif
}

static ssize_t _PREAD64(const char *op, int fd, const void *buf, size_t size, off64_t off)
{
	if (pwrite64_init() == -1)
		return -1;

	ssize_t ret = _pwrite64(fd, buf, size, off);

#ifdef TEST_OPS
	if (ret > 0 && fd_to_path(fd, pwrite_path) != -1 &&
		strncmp(pwrite_path, "/sys/", strlen("/sys/")) &&
		strncmp(pwrite_path, "/proc/", strlen("/proc/")) &&
		strncmp(pwrite_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(pwrite_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"pwrite64\",\"Path\":\"%s\"", pwrite_path);
#endif

	if (ret > 0 && _notify) {
#ifdef TEST_OPS
		if (strncmp(pwrite_path, "/sys/", strlen("/sys/")) &&
			strncmp(pwrite_path, "/proc/", strlen("/proc/")) &&
			strncmp(pwrite_path, "pipe:[", strlen("pipe:[")) &&
			strncmp(pwrite_path, "/dev/", strlen("/dev/")))
			test("\"Stage\":2,\"Op\":\"pwrite64\",\"Path\":\"%s\"", pwrite_path);
#endif

#ifdef PER_FILE_BATCHING
		if (last_pwrite64_fd == fd &&
			!batch_read_write_first(op, pwrite_path, &last_pwrite64_fd, &last_pwrite64_size,
										&last_pwrite64_off, fd, ret, off, pwrite64_iostr)) {
			return ret;
		}
#endif

		/* different file, dispatch batched read notifications */
		if (fd_to_path(fd, pwrite_path) != -1 && notify(fd, S_IFREG, pwrite_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"pwrite64\",\"Path\":\"%s\"", pwrite_path);
#endif
#ifdef PER_FILE_BATCHING
			batch_read_write_second(op, pwrite_path, &last_pwrite64_fd, &last_pwrite64_size,
										&last_pwrite64_off, fd, ret, off, pwrite64_iostr);
#else
			notify_read_write(op, pwrite_path, fd, size, off, NULL);
#endif
		}
	}

	return ret;
}

ssize_t pwrite64(int fd, const void *buf, size_t size, off64_t off)
{
	return _PREAD64("pwrite64", fd, buf, size, off);
}

ssize_t pwrite(int fd, const void* buf, size_t size, off_t off)
{
	return _PREAD64("pwrite", fd, buf, size, (off64_t)off);
}

