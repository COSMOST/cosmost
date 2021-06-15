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

static __thread char fwrite_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_fwrite_fd = -1;
static __thread off64_t last_fwrite_off = -1;
static __thread size_t last_fwrite_size = 0;
static __thread char fwrite_iostr[IOSTR_BUFSIZE];
#endif

static fwrite_t _fwrite;

static int fwrite_init(void)
{
	if (!_fwrite) {
		_fwrite = (fwrite_t) dlsym(RTLD_NEXT, "fwrite");
		if (!_fwrite)
    		_fwrite = (fwrite_t) dlsym(RTLD_NEXT, "__fwrite");
	}

	if (!_fwrite) {
		errno = EIO;
		ALOGE("NULL _fwrite!");
		return -1;
	}

	return 0;
}

void fwrite_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	if ((fd != -1 && fd == last_fwrite_fd) ||
		(fd == -1 && last_fwrite_fd != -1)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"fwrite\",\"Path\":\"%s\"", fwrite_path);
#endif

		info("\"op\":\"%s\",\"last_fwrite_fd\":%d,\"last_fwrite_size\":%llu,"
			"\"last_fwrite_off\":%lld",
			fd == -1 ? "fwrite (exit)" : "fwrite (close)",
			last_fwrite_fd, (uint64_t)last_fwrite_size, (int64_t)last_fwrite_off);

		notify_read_write(fd == -1 ? "fwrite (exit)" : "fwrite (close)",
							fwrite_path, last_fwrite_fd,
							last_fwrite_size, last_fwrite_off, fwrite_iostr);

		last_fwrite_fd = -1;
	}
#endif
}

size_t fwrite(const void *buf, size_t size, size_t count, FILE *fp)
{
	if (fwrite_init() == -1)
		return -1;

	size_t ret = _fwrite(buf, size, count, fp);

#ifdef TEST_OPS
	if (!ferror(fp) && ret > 0 && fd_to_path(fileno(fp), fwrite_path) != -1 &&
		strncmp(fwrite_path, "/sys/", strlen("/sys/")) &&
		strncmp(fwrite_path, "/proc/", strlen("/proc/")) &&
		strncmp(fwrite_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(fwrite_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"fwrite\",\"Path\":\"%s\"", fwrite_path);
#endif

	if (!ferror(fp) && ret > 0 && _notify) {
		int fd = fileno(fp);

#ifdef TEST_OPS
		if (fd != -1 && strncmp(fwrite_path, "/sys/", strlen("/sys/")) &&
			strncmp(fwrite_path, "/proc/", strlen("/proc/")) &&
			strncmp(fwrite_path, "pipe:[", strlen("pipe:[")) &&
			strncmp(fwrite_path, "/dev/", strlen("/dev/")))
			test("\"Stage\":2,\"Op\":\"fwrite\",\"Path\":\"%s\"", fwrite_path);
#endif

#ifdef PER_FILE_BATCHING
		if (fd != -1 && last_fwrite_fd == fd) {
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, fwrite_path);
			else if (!batch_read_write_first("fwrite", fwrite_path, &last_fwrite_fd, &last_fwrite_size,
											&last_fwrite_off, fd, ret, off, fwrite_iostr)) {
				return ret;
			}
		}
#endif

		if (fd_to_path(fd, fwrite_path) != -1 && notify(fd, S_IFREG, fwrite_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"fwrite\",\"Path\":\"%s\"", fwrite_path);
#endif
			off64_t off = lseek64(fd, 0, SEEK_CUR) - ret;
			if (off < 0)
				ALOGE("%s: err: %s fd: %d off: %llu ret: %llu path: %s",
						__func__, strerror(errno), fd, (int64_t)off, (uint64_t)ret, fwrite_path);
			else {
#ifdef PER_FILE_BATCHING
				batch_read_write_second("fwrite", fwrite_path, &last_fwrite_fd, &last_fwrite_size,
											&last_fwrite_off, fd, ret, off, fwrite_iostr);
#else
				notify_read_write("fwrite", fwrite_path, fd, size * count, off, NULL);
#endif
			}
		}
	}

	return ret;
}

