#ifndef __STUDY_FS_NOTIFY_H__
#define __STUDY_FS_NOTIFY_H__

int notify_read(const char *, const char *, int, size_t, off_t);
int notify_close(int);
int notify_write(const char *, const char *, int, size_t, off_t);
int notify_create(const char *, const char *);
int notify_unlink(const char *, const char *);
int notify_rename(const char *, const char *, const char *);
int notify_init(void);
void notify_exit(void);

#endif /* __STUDY_FS_NOTIFY_H__ */
