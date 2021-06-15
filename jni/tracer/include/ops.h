#ifndef __STUDY_OPS_H__
#define __STUDY_OPS_H__

#include <pthread.h>

typedef __attribute__((noreturn)) void (*pthread_exit_t)(void *);
typedef int (*pthread_create_t)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

typedef int (*clone_t)(int (*fn)(void*), void*, int, void*, ...);

typedef int (*atexit_t)(void (*func)(void));

typedef FILE* (*fopen_t)(const char *, const char *);
typedef FILE* (*fdopen_t)(int, const char *);
typedef int (*fclose_t)(FILE *);

typedef int (*unlink_t)(const char *);
typedef int (*unlinkat_t)(int, const char *, int);

typedef size_t (*fread_t)(void *, size_t, size_t, FILE *);
typedef size_t (*fwrite_t)(const void *, size_t, size_t, FILE *);

typedef int (*open_t)(const char *, int, ...);
typedef int (*openat_t)(int, const char *, int, ...);
typedef int (*close_t)(int);

typedef ssize_t (*read_t)(int, void *, size_t);
typedef ssize_t (*write_t)(int, const void *, size_t);

typedef ssize_t (*pread64_t)(int, void *, size_t, off64_t);
typedef ssize_t (*pwrite64_t)(int, const void *, size_t, off64_t);

typedef int (*rename_t)(const char *, const char *);
typedef int (*renameat_t)(int, const char *, int, const char *);

typedef void* (*mmap64_t)(void*, size_t, int, int, int, off64_t);
typedef int (*munmap_t)(void*, size_t);
#endif /* __STUDY_OPS_H__ */
