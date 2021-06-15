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
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static __thread char fread_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_fread_fd = -1;
static __thread off64_t last_fread_off = -1;
static __thread size_t last_fread_size = 0;
static __thread char fread_iostr[IOSTR_BUFSIZE];
#endif

static fread_t _fread;

static int fread_init(void)
{
	if (!_fread) {
		_fread = (fread_t) dlsym(RTLD_NEXT, "fread");
		if (!_fread)
    		_fread = (fread_t) dlsym(RTLD_NEXT, "__fread");
	}

	if (!_fread) {
		errno = EIO;
		err("\"_fread\":0");
		return -1;
	}

	return 0;
}

void fread_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	if ((fd != -1 && fd == last_fread_fd) ||
		(fd == -1 && last_fread_fd != -1)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"fread\",\"Path\":\"%s\"", fread_path);
#endif

		info("\"op\":\"%s\",\"last_fread_fd\":%d,\"last_fread_size\":%llu,"
			"\"last_fread_off\":%lld",
			fd == -1 ? "fread (exit)" : "fread (close)",
			last_fread_fd, (uint64_t)last_fread_size, (int64_t)last_fread_off);

		notify_read_write(fd == -1 ? "fread (exit)" : "fread (close)",
							fread_path, last_fread_fd,
							last_fread_size, last_fread_off, fread_iostr);

		last_fread_fd = -1;
	}
#endif
}

size_t fread(void *buf, size_t size, size_t count, FILE *fp)
{
	if (fread_init() == -1)
		return -1;

	size_t ret = _fread(buf, size, count, fp);

#ifdef TEST_OPS
	if (ret > 0 && ret > 0 && fileno(fp) != -1 &&
		fd_to_path(fileno(fp), fread_path) != -1 &&
		strncmp(fread_path, "/sys/", strlen("/sys/")) &&
		strncmp(fread_path, "/proc/", strlen("/proc/")) &&
		strncmp(fread_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(fread_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"fread\",\"Path\":\"%s\"", fread_path);
#endif

	if (!ferror(fp) && ret > 0 && _notify) {
		int fd = fileno(fp);

#ifdef TEST_OPS
	if (fd != -1 && strncmp(fread_path, "/sys/", strlen("/sys/")) &&
		strncmp(fread_path, "/proc/", strlen("/proc/")) &&
		strncmp(fread_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(fread_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":2,\"Op\":\"fread\",\"Path\":\"%s\"", fread_path);
#endif

#ifdef PER_FILE_BATCHING
		if (fd != -1 && last_fread_fd == fd) {
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, fread_path);
			else if (!batch_read_write_first("fread", fread_path, &last_fread_fd, &last_fread_size,
											&last_fread_off, fd, ret, off, fread_iostr)) {
				return ret;
			}
		}
#endif

		if (fd_to_path(fd, fread_path) != -1 && notify(fd, S_IFREG, fread_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"fread\",\"Path\":\"%s\"", fread_path);
#endif

			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, fread_path);
			else {
#ifdef PER_FILE_BATCHING
				batch_read_write_second("fread", fread_path, &last_fread_fd, &last_fread_size,
											&last_fread_off, fd, ret, off, fread_iostr);
#else
				notify_read_write("fread", fread_path, fd, size * count, off, NULL);
#endif
			}
		}
	}

	return ret;
}

