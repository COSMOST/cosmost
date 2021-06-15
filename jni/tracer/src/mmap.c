#define LOG_TAG "Storage-IO-UIUC"

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
#include <sys/mman.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static __thread char mmap_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_mmap_fd = -1;
static __thread off64_t last_mmap_off = -1;
static __thread size_t last_mmap_size = 0;
#endif

static mmap64_t _mmap64;
static int mmap_init(void)
{
	if (!_mmap64) {
		_mmap64 = (mmap64_t) dlsym(RTLD_NEXT, "mmap64");
		if (!_mmap64)
    		_mmap64 = (mmap64_t) dlsym(RTLD_NEXT, "__mmap64");
	}

	if (!_mmap64) {
		errno = EIO;
		ALOGE("NULL _mmap!");
		return -1;
	}

	return 0;
}

void mmap_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	/* file closing or thread exiting */
	if ((fd == -1 && last_mmap_fd != -1) ||
		(fd != -1 && fd == last_mmap_fd)) {

		info("\"op\":\"%s\",\"last_mmap_fd\":%d,\"last_mmap_size\":%llu,"
			"\"last_mmap_off\":%lld",
			fd == -1 ? "mmap (exit)" : "mmap (close)",
			last_mmap_fd, (uint64_t)last_mmap_size, (int64_t)last_mmap_off);

		last_mmap_fd = -1;
	}
#endif
}

static __thread char protstr[8];
static void prot_str(int prot, int flags)
{
	protstr[0] = prot & PROT_READ ? 'R' : '-';
	protstr[1] = prot & PROT_WRITE ? 'W' : '-';
	protstr[2] = prot & PROT_EXEC ? 'X' : '-';
	protstr[3] = flags & MAP_SHARED ? 'S' :
					flags & MAP_PRIVATE ? 'P' : '-';
	protstr[4] = '\0';
}

static void* _MMAP64(const char *op, void *addr, size_t length, int prot,
						int flags, int fd, off64_t offset)
{
	if (mmap_init() == -1)
		return NULL;

	/* actual syscall */
	void *ret = _mmap64(addr, length, prot, flags, fd, offset);

	if (ret != MAP_FAILED && _notify && fd != -1 &&
		!(flags & MAP_ANONYMOUS) && fd_to_path(fd, mmap_path) != -1 &&
		notify(fd, S_IFREG, mmap_path)) {

		prot_str(prot, flags);
		notify_mmap(op, mmap_path, ret, protstr, length, offset);
	}

	return ret;
}

void* mmap64(void *addr, size_t size, int prot, int flags,
				int fd, off64_t offset)
{
	return _MMAP64("mmap64", addr, size, prot, flags, fd, offset);
}

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset)
{
	return _MMAP64("mmap", addr, size, prot, flags, fd, (off64_t)offset);
}

#if 0
static munmap_t _munmap;
static int munmap_init(void)
{
	if (!_munmap) {
		_munmap = (munmap_t) dlsym(RTLD_NEXT, "munmap");
		if (!_munmap)
    		_munmap = (munmap_t) dlsym(RTLD_NEXT, "__munmap");
	}

	if (!_munmap) {
		errno = EIO;
		ALOGE("NULL _munmap!");
		return -1;
	}

	return 0;
}

int munmap(void* addr, size_t length)
{
	if (munmap_init() == -1)
		return -1;

	int ret = _munmap(addr, length);
	if (ret != -1 && _notify)
		notify_munmap("munmap", addr, length);

	return ret;
}
#endif
