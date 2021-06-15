#ifndef __STUDY_FS_NOTIFY_H__
#define __STUDY_FS_NOTIFY_H__

#include <sys/stat.h>

#ifdef TEST_FILE_BATCHING
__attribute__((visibility("hidden")))
int test_read_write_batch(const char *, const char *, int,
						size_t, off64_t);
#endif

#ifdef BUFFER_BATCHING
__attribute__((visibility("hidden")))
int batched_blog(const char *, ...);
#endif

__attribute__((visibility("hidden")))
int notify_close(const char *);

__attribute__((visibility("hidden")))
int notify_read_write(const char *, const char *, int,
						size_t, off64_t, char *);

__attribute__((visibility("hidden")))
int notify_munmap(const char *, void *, size_t);

__attribute__((visibility("hidden")))
int notify_mmap(const char *, const char *, void *,
				const char *, size_t, off64_t);

__attribute__((visibility("hidden")))
int notify_create(const char *, const char *);

__attribute__((visibility("hidden")))
int notify_unlink(const char *, int, const char *);

__attribute__((visibility("hidden")))
int notify_rename(const char *, int, const char *, const char *);

__attribute__((visibility("hidden")))
void notify_thread_init(int);

__attribute__((visibility("hidden")))
void notify_thread_exit(int);

#endif /* __STUDY_FS_NOTIFY_H__ */
