#ifndef __STUDY_FS_COMMON_H__
#define __STUDY_FS_COMMON_H__

void pthread_common(int, int);
void close_common(int);
void read_close(int);
void fread_close(int);
void pread64_close(int);
void mmap_close(int);
void write_close(int);
void fwrite_close(int);
void pwrite64_close(int);
#endif /* __STUDY_FS_COMMON_H__ */
