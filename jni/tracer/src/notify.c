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

#include <cutils/log.h>
#include <log/logger.h>

#include "ops.h"
#include "fs.h"
#include "notify.h"

#ifdef BUFFER_BATCHING
#include "bblog.h"
#define BLOG(fmt, ...) (void)bblog(fmt, ##__VA_ARGS__)
#else
#include "blog.h"
#define BLOG(fmt, ...) (void)blog(fmt, ##__VA_ARGS__)
#endif

#define mode(flags)	\
	(((flags & S_IFMT) == (S_IFREG | S_IFDIR)) ? "FILE_OR_DIR" :	\
	((flags & S_IFMT) == S_IFREG) ? "FILE" :						\
	((flags & S_IFMT) == S_IFDIR) ? "DIR" :	"UNKNOWN")

int notify_munmap(const char *op, void *addr, size_t size)
{
    struct timeval t;
	gettimeofday(&t, NULL);

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"TS\":%li.%06li,"
			"\"ADDR\":%llu,\"SIZE\":%llu}",
			gettid(), op, t.tv_sec, t.tv_usec,
			(uint64_t)(uintptr_t)addr, size);

	return 0;
}

int notify_mmap(const char *op, const char *path, void *addr,
				const char *prot, size_t size, off64_t off)
{
    struct timeval t;
	gettimeofday(&t, NULL);

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"PATH\":\"%s\",\"TS\":%li.%06li,"
			"\"ADDR\":%llu,\"SIZE\":%llu,\"OFF\":%lld,\"PROT\":\"%s\"}",
			gettid(), op, path, t.tv_sec, t.tv_usec,
			(uint64_t)(uintptr_t)addr, (uint64_t)size, (int64_t)off, prot);

	return 0;
}

int notify_read_write(const char *op, const char *path,
						int fd, size_t size, off64_t off, char *str)
{
	struct timeval t;
	gettimeofday(&t, NULL);

#ifdef PER_FILE_BATCHING
	/* formatting needs (remove last ';') */
	if (str)
		str[strlen(str) - 1] = '\0';

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"PATH\":\"%s\",\"TS\":%li.%06li,"
			"\"SIZE\":%llu,\"OFF\":%lld,\"RW\":\"%s\"}",
			gettid(), op, path, t.tv_sec, t.tv_usec,
			(uint64_t)size, (int64_t)off, str ? str : "NA");
#else
	(void)str;	
	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"PATH\":\"%s\",\"TS\":%li.%06li,"
			"\"SIZE\":%llu,\"OFF\":%lld}",
			gettid(), op, path, t.tv_sec, t.tv_usec,
			(uint64_t)size, (int64_t)off);
#endif
	
	return 0;
}

int notify_unlink(const char *op, int flags, const char *path)
{
    struct timeval t;
	gettimeofday(&t, NULL);

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"MODE\":\"%s\","
			"\"PATH\":\"%s\",\"TS\":%li.%06li}",
			gettid(), op, mode(flags), path, t.tv_sec, t.tv_usec);

	return 0;
}

int notify_rename(const char *op, int flags, const char *old, const char *new)
{
    struct timeval t;
	gettimeofday(&t, NULL);

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"MODE\":\"%s\",\"OLD\":\"%s\","
			"\"NEW\":\"%s\",\"TS\":%li.%06li}",
			gettid(), op, mode(flags), old, new, t.tv_sec, t.tv_usec);

	return 0;
}

int notify_close(const char *path)
{
	(void)path;
	return 0;
}

int notify_create(const char *op, const char *path)
{
    struct timeval t;
	gettimeofday(&t, NULL);

	BLOG("{\"TID\":%d,\"OP\":\"%s\",\"PATH\":\"%s\","
			"\"TS\":%li.%06li}",
			gettid(), op, path, t.tv_sec, t.tv_usec);

	return 0;
}

void notify_thread_init(int main)
{
#ifdef BUFFER_BATCHING
	bblog_init(main);
#endif

	if (main == 1) {
#ifdef WRITE_TO_FILE
		override_defaults();
#endif
	}
}

/*
 * Parameter decoding:
 *
 * -tive @main means signal delivered
 * (signal num = abs val of @main),
 *
 * main = 1 means process extiting
 * main = 0 means thread exiting
 */
void notify_thread_exit(int main)
{
#ifdef BUFFER_BATCHING
	bblog_exit(main);
#endif

	if (main == 1) {
#ifdef WRITE_TO_FILE
		if (thread_fd != -1)
			_close(thread_fd);
#endif
	}
}
