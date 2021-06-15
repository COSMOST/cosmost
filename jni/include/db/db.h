#ifndef __STUDY_FILEDB_H__
#define __STUDY_FILEDB_H__

#ifdef __cplusplus
extern "C" {
#endif

int open_database(const char *);
int add_entry(const char *);
int remove_entry(const char *);
int update_entry(const char *, const char *);

#ifdef __cplusplus
}
#endif

#endif /* __STUDY_FILEDB_H__ */
