#ifndef __STUDY_FS_DB_H__
#define __STUDY_FS_DB_H__
int db_create(struct timeval *, const char *);
int db_unlink(struct timeval *, const char *);
int db_rename(struct timeval *, const char *, const char *);
int db_read(struct timeval *, const char *, size_t, off_t);
int db_write(struct timeval *, const char *, size_t, off_t);
int db_init(void);
#endif /* __STUDY_FS_DB_H__ */
