#ifndef __STUDY_FLUSHER_H__
#define __STUDY_FLUSHER_H__
#ifdef FLUSHER_THREAD
int start_flusher_thread(uint64_t **, uint64_t *);
void stop_flusher_thread(void);
#endif
#endif
