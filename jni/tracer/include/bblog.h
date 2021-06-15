#ifndef __STUDY_BBLOG_H__
#define __STUDY_BBLOG_H__

#ifdef BUFFER_BATCHING

#define PID_MAX 32767

#define MAX_PREFIX_LEN	512

#define PIBR_IDX_SHIFT	32
#define PIBR_IDX_MASK	0xffffUL

#define PIBR_PID_SHIFT		48
#define PIBR_REFCNT_MASK	0xfffUL

#define PIBR_PID(x)		(pid_t)(x >> PIBR_PID_SHIFT)
#define PIBR_IDX(x)		((uint16_t)((x >> PIBR_IDX_SHIFT) & PIBR_IDX_MASK))
#define PIBR_REFCNT(x)	(uint16_t)(x & PIBR_REFCNT_MASK)
#define PIBR_PTR(x)		(void *)((uint32_t)((uint32_t)x & ~PIBR_REFCNT_MASK))

/*
 * Lowest 32-bits contain active buf_t (with last 12-bits containing refcnt)
 * Highest 32-bits contain pid that active buf_t belongs to
 *
 * XXX works only on 32-bit little-endian machine
 */
#define PIBR_BUF_SIZE (LOGGER_ENTRY_MAX_PAYLOAD - sizeof(uint64_t))
typedef struct {
	uint64_t pibr;
	char buf[PIBR_BUF_SIZE];
} __attribute__((packed)) pibr_t;

extern uint64_t last_bblog_secs;
extern uint64_t proc_buf_pid;

void swap_and_flush_buffer(uint64_t *, pid_t);

int bblog(const char*, ...);

void bblog_init(int);
void bblog_exit(int);
#endif
#endif

