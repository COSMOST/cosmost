#ifdef PER_FILE_BATCHING
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

#include "process.h"
#include "fs.h"
#include "notify.h"
#include "batch.h"

#undef ALOGD
#define ALOGD(...)

#define BLKSIZE_SHIFT	12
#define BLKSIZE			(1 << BLKSIZE_SHIFT)

#define BATCH_THRESHOLD	5

static void batch_read_write_notify(const char *op, const char *path, int fd,
										int *last_fd, size_t *last_size, off64_t *last_off,
										off64_t curr_start, off64_t curr_end,
										size_t size, off64_t off, char *access)
{
	if (*last_fd != -1 && *last_size)
		notify_read_write(op, path, *last_fd, *last_size, *last_off, access);

	/* batch per-file read notifications */
	*last_fd = fd;
	*last_off = curr_start;
	*last_size = curr_end - curr_start + 1;

	snprintf(access, IOSTR_BUFSIZE, "%llu@%llu;", (uint64_t)size, (uint64_t)off);

#ifdef TEST_FILE_BATCHING
	test_read_write_batch(op, path, fd, size, off);
#endif
}

static void record_iostr(const char *op, const char *path, int fd,
							int *last_fd, size_t *last_size, off64_t *last_off,
							off64_t curr_start, off64_t curr_end,
							size_t size, off64_t off, char *access)
{
	/* record I/O access string for batched blocks */
	if (access) {
		int idx = strlen(access);

		/* protect against buffer overflow */
		if ((idx + 64) >= IOSTR_BUFSIZE) /*|| *last_size > BATCH_THRESHOLD)*/
			batch_read_write_notify(op, path, fd, last_fd, last_size, last_off,
										curr_start, curr_end, size, off, access);
		else {
			snprintf(&access[idx], IOSTR_BUFSIZE - idx, "%llu@%llu;", (uint64_t)size, (uint64_t)off);

#ifdef TEST_FILE_BATCHING
			test_read_write_batch(op, path, fd, size, off);
#endif
		}
	}
}

void batch_read_write_second(const char *op, char *path,
							int *last_fd, size_t *last_size, off64_t *last_off,
							int fd, size_t size, off64_t off, char *access)
{
	if (!op || !path || fd < 0 || !size || off < 0 ||
		!last_fd || !last_size || !last_off || !access) {
		ALOGE("%s: invalid args op: %p path: %p fd: %d size: %llu off: %lld "
				"last_fd: %p last_size: %p last_off: %p access: %p",
				__func__, op, path, fd, (uint64_t)size, (int64_t)off,
				last_fd, last_size, last_off, access);
		return;
	}

	off64_t last_start = *last_off;
	off64_t last_end = *last_off + *last_size - 1;

#ifdef BATCH_BLOCKS
	// starting and ending block numbers
	off64_t curr_start = (off >> BLKSIZE_SHIFT);
	off64_t curr_end = ((off + size - 1 + BLKSIZE - 1) >> BLKSIZE_SHIFT);
#else
	off64_t curr_start = off;
	off64_t curr_end = off + size - 1;
#endif

	off64_t curr_size = (curr_end - curr_start + 1);

	ALOGD("OP: %s Proc: %s: TID: %d fd = %d last_fd = %d "
			"last_start = %lld last_end = %lld last_size = %llu "
			"curr_start = %lld curr_end = %lld curr_size = %llu "
			"off = %lld size = %llu",
			op, process_name(), gettid(), fd, *last_fd,
			(int64_t)last_start, (int64_t)last_end, (uint64_t)*last_size,
			(int64_t)curr_start, (int64_t)curr_end, (uint64_t)curr_size,
			(int64_t)off, (uint64_t)size);

	batch_read_write_notify(op, path, fd, last_fd, last_size, last_off,
								curr_start, curr_end, size, off, access);
}

int batch_read_write_first(const char *op, char *path,
							int *last_fd, size_t *last_size, off64_t *last_off,
							int fd, size_t size, off64_t off, char *access)
{
	if (!op || !path || fd < 0 || !size || off < 0 ||
		!last_fd || !last_size || !last_off || !access) {
		ALOGE("%s: invalid args op: %p path: %p fd: %d size: %llu off: %lld "
				"last_fd: %p last_size: %p last_off: %p access: %p",
				__func__, op, path, fd, (uint64_t)size, (int64_t)off,
				last_fd, last_size, last_off, access);
		return -1;
	}

	off64_t last_start = *last_off;
	off64_t last_end = *last_off + *last_size - 1;

#ifdef BATCH_BLOCKS
	// starting and ending block numbers
	off64_t curr_start = (off >> BLKSIZE_SHIFT); // floor(blkno)
	off64_t curr_end = ((off + size - 1 + BLKSIZE - 1) >> BLKSIZE_SHIFT); // ceil(blkno)
#else
	off64_t curr_start = off;
	off64_t curr_end = off + size - 1;
#endif

	off64_t next = last_end + 1;
	off64_t prev = last_start - 1;
	off64_t curr_size = (curr_end - curr_start + 1);

	ALOGD("OP: %s Proc: %s: TID: %d fd = %d last_fd = %d "
			"last_start = %lld last_end = %lld last_size = %llu "
			"curr_start = %lld curr_end = %lld curr_size = %llu "
			"off = %lld size = %llu",
			op, process_name(), gettid(), fd, *last_fd,
			(int64_t)last_start, (int64_t)last_end, (uint64_t)*last_size,
			(int64_t)curr_start, (int64_t)curr_end, (uint64_t)curr_size,
			(int64_t)off, (uint64_t)size);

	/* starting after @last_end or ending before @last_start */
	if (curr_start > next || curr_end < prev) {
		ALOGD("TID: %d IV %s: an offset beyond @last_end or earlier than "
				"@last_start and ends before @last_start", gettid(), op);
		batch_read_write_notify(op, path, fd, last_fd, last_size, last_off,
									curr_start, curr_end, size, off, access);
		return 0;
	}

	record_iostr(op, path, fd, last_fd, last_size, last_off,
					curr_start, curr_end, size, off, access);

	/* do nothing for anything between (@last_start, @last_end) */
	if (curr_start >= last_start && curr_end <= last_end)
		return 0;

	/* starting between (@last_start, @last_end) and ending beyond @last_end */
	if (curr_start >= last_start && curr_start <= next && curr_end >= next) {
		*last_size += (curr_end - last_end);
		ALOGD("TID: %d I %s: from between (@last_start, @last_end), but beyond "
				"@last_end", gettid(), op);
		return 0;
	}

	/* starting before @last_start and ending beyond @last_end */
	if (curr_start <= last_start && curr_end >= last_end) {
		*last_off = curr_start;
		*last_size = curr_size;
		ALOGD("TID: %d II %s: from an earlier offset and beyond @last_end",
				gettid(), op);
		return 0;
	}

	/* starting before @last_start and ending between (@last_start, @last_end) */
	if (curr_start < last_start && curr_end >= prev && curr_end <= last_end) {
		*last_off = curr_start;
		*last_size += (last_start - curr_start);
		ALOGD("TID: %d III %s: from an earlier offset, but between (@last_start, "
				"@last_end)", gettid(), op);
		return 0;
	}

	return -1;
}
#endif
