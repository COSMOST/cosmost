#ifndef __STUDY_FS_BATCH_H__
#define __STUDY_FS_BATCH_H__

typedef struct {
	const char *op;
	const char *path;
	int *last_fd;
	size_t *last_size;
	off64_t *last_off;
	int fd;
	size_t size;
	off64_t off;
	char *access;
} batch_read_write_t;

__attribute__((visibility("hidden"))) uint64_t batch_counter(void);

__attribute__((visibility("hidden")))
ssize_t batch_read_write_first(const char*, char*,
                            int*, size_t*, off64_t*,
                            int, size_t, off64_t, char*);

__attribute__((visibility("hidden")))
void batch_read_write_second(const char*, char*,
                            int*, size_t*, off64_t*,
                            int, size_t, off64_t, char*);

#endif /* __STUDY_FS_BATCH_H__ */
