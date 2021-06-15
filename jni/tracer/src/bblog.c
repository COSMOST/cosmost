#ifdef BUFFER_BATCHING
#define LOG_TAG "Storage-IO-UIUC"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#include "list.h"
#include "ops.h"
#include "fs.h"
#include "batch.h"
#include "counter.h"
#include "bblog.h"

#ifdef FLUSHER_THREAD
#include "flusher.h"
#elif FLUSH_TIMER
#include "timer.h"
#endif

#ifdef WRITE_TO_FILE
static int log_fd = -1;
static openat_t _openat;
static write_t _write;
static close_t _close;
#endif

//#undef info
//#define info err

/* XXX @buffer must be a string ending with a '\0' char */
static inline int no_batch_blog(const char *buffer)
{
	#define NOBATCH_LOG_TAG "Storage-IONB-UIUC"
	/* XXX order is reversed becase @buffer contains an
	 * ending ',' for JSON fomatting */
	char large_buf[LOGGER_ENTRY_MAX_PAYLOAD];
	snprintf(large_buf, LOGGER_ENTRY_MAX_PAYLOAD,
								"{\"PID\":%d,\"PROC\":\"%s\",\"BATCH\""
								":{%s\"NOBATCH\":\"YES\"}}",
								getpid(), process_name(), buffer);
	return __android_log_write(ANDROID_LOG_INFO, NOBATCH_LOG_TAG,
									large_buf);
}

static char prefix[MAX_PREFIX_LEN];
static int prefix_len;
static pid_t procid;

static uint64_t *proc_pibr;

static int reinit_procid(pid_t new_pid)
{
	/* check if process-wide (re)initialization is needed */
	pid_t old_pid;
	while (1) {

		old_pid = __atomic_load_n (&procid, __ATOMIC_SEQ_CST);
		if (old_pid == new_pid)
			break; // not needed

		/* perform process-wide (re)initialization */
		prefix_len = snprintf(prefix, MAX_PREFIX_LEN,
								"{\"PID\":%d,\"PROC\":\"%s\",\"BATCH\":{",
								new_pid, process_name());

		if (prefix_len >= MAX_PREFIX_LEN) {
			err("\"Prefix too long\":%d", prefix_len);
			return -1;
		}

		if (__sync_bool_compare_and_swap(&procid, old_pid, new_pid)) {
#ifdef FLUSH_TIMER
			stop_flusher_timer();
			start_flusher_timer(&proc_pibr);
#endif
			break;
		}
	}

	return 0;
}

/* assuming @pid is 16-bits, @buf is 32-bits, and @idx is 16-bits wide */
static inline uint64_t
pack_buffer(void *ptr, pid_t pid, uint16_t idx, uint16_t refcnt)
{
	if (pid < 0 || pid > PID_MAX) {
		err("\"Invalid PID\":%d", pid);
		return 0;
	}

	uint64_t packed = ((uint64_t)pid << PIBR_PID_SHIFT) |
						((uint64_t)idx << PIBR_IDX_SHIFT) |
						((uint32_t)ptr & ~PIBR_REFCNT_MASK) |
						(refcnt & PIBR_REFCNT_MASK);

	info("\"ptr\":%p,\"idx\":0x%x,\"refcnt\":%d,\"pid\":%d,\"packed\":0x%llx",
		ptr, idx, refcnt, pid, packed);

	return packed;
}

static void init_buffer(pibr_t *ptr, pid_t pid)
{
	/* re-initialize process prefix if needed */
	if (reinit_procid(pid) == -1)
		return;

	/* copy process prefix */
	strcpy(ptr->buf, prefix);

	/* prefix_len points to next free position in @ptr */
	uint64_t pibr = pack_buffer(ptr, pid, prefix_len, 0);
	ptr->pibr = pibr;

	info("\"pibr\":0x%llx,\"pid\":%d,\"buf\":%p,\"refcnt\":%d,\"idx\":0x%x",
			pibr, PIBR_PID(pibr), PIBR_PTR(pibr), PIBR_REFCNT(pibr), PIBR_IDX(pibr));
}

static pibr_t* alloc_buffer(pid_t pid)
{
	/* buf must contain last 12-bits as 0s */
	pibr_t *ptr = memalign(1 + PIBR_REFCNT_MASK, sizeof(pibr_t));
	if (!ptr) {
		err("\"Failed to alloc pibr\":\"%s\"", strerror(errno));
		return 0;
	}

	if ((uintptr_t)ptr & PIBR_REFCNT_MASK) {
		err("\"Failed to alloc aligned buffer\":\"%s\"", strerror(errno));
		free(ptr);
		return 0;
	}

	init_buffer(ptr, pid);
	//add_list_tail(ptr);

	return ptr;
}

/* timestamp when last flush buffer was done */
static uint64_t last_flush_secs;

static void flush_buffer(pibr_t *ptr)
{
	if (!ptr)
		return;

	info("\"Flushing buffer\":%p", ptr);

	/* check for pid match (avoid stale buffer flush) */
	pid_t pid = PIBR_PID(ptr->pibr);
	if (!__sync_bool_compare_and_swap(&procid, pid, pid))
		return;

	/* check if there is something useful to flush */
	uint16_t idx = PIBR_IDX(ptr->pibr);
	if (__sync_bool_compare_and_swap(&prefix_len, idx, idx))
		return;

	/* no other thread (except this) must be holding refcnt on buffer */
	uint64_t pibr = pack_buffer(ptr, pid, idx, 0);

	while (!__sync_bool_compare_and_swap(&ptr->pibr, pibr, 0))
		;

	info("\"Flushing buffer\":%p,\"Strlen(buffer)\":%d",
			ptr, strlen(ptr->buf));

	strcpy(&ptr->buf[idx - 1], "}}");
	__android_log_write(ANDROID_LOG_INFO, LOG_TAG, ptr->buf);

	info("\"Freeing buffer\":%p,\"Strlen(buffer)\":%d",
			ptr, strlen(ptr->buf));

	/* record curr time for flusher_thread */
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	__atomic_store_n (&last_flush_secs, ts.tv_sec, __ATOMIC_SEQ_CST);

	free(ptr);
}

/* returns 0 if swap failed, 1 for success */
static int swap_buffers(uint64_t *curr_pibr, pid_t new_pid)
{
	/* if already swapped */
	if (!__sync_bool_compare_and_swap(&proc_pibr, curr_pibr, curr_pibr))
		return 0;

	pibr_t *new_ptr = alloc_buffer(new_pid);
	if (!new_ptr) {
		/* fallback to non-batching mode */
		__sync_fetch_and_and(&proc_pibr, 0);
		return 1;
	}

	if (__sync_bool_compare_and_swap(&proc_pibr, curr_pibr,
									new_ptr ? &new_ptr->pibr : 0)) {
		info("swapped: \"old_pibr\":0x%llx,\"new_pibr\":0x%llx",
				curr_pibr ? *curr_pibr : 0,
				new_ptr ? new_ptr->pibr : 0);
		return 1;
	}

	free(new_ptr);
	return 0;
}

/* swaps existing buffer and flushes it */
void swap_and_flush_buffer(uint64_t *curr_pibr, pid_t new_pid)
{
	int ret = swap_buffers(curr_pibr, new_pid);
	if (ret && curr_pibr)
		flush_buffer(PIBR_PTR(curr_pibr));
}

static int _blog(char *buffer, size_t size)
{
	pibr_t *ptr;
	uint16_t idx;
	uint64_t curr_pibr;

	pid_t pid, new_pid = getpid();
	uint64_t incr = pack_buffer(0, 0, size, 1);

	while (1) {
		/* get current values */
		curr_pibr = __sync_fetch_and_add(proc_pibr, incr);

		ptr = PIBR_PTR(curr_pibr);

		/* test for stale pids (forked processes) */
		if (!ptr) {
			__sync_fetch_and_sub(&ptr->pibr, incr);
			size = no_batch_blog(buffer);
			break;
		}

		info("\"Old_PID\":%d,\"New_PID\":%d,"
				"\"curr_pibr\":0x%llx,\"idx\":%d,\"ptr\":%p",
				pid, new_pid, curr_pibr, idx, ptr);

		pid = PIBR_PID(curr_pibr);
		idx = PIBR_IDX(curr_pibr);

		asm volatile("" ::: "memory");

		if (pid == new_pid &&
			(idx + size) < (LOGGER_ENTRY_MAX_PAYLOAD - 32)) {
			memcpy((void *)&ptr->buf[idx], buffer, size);
			__sync_fetch_and_sub(&ptr->pibr, 1);
			break;
		}
		__sync_fetch_and_sub(&ptr->pibr, incr);

		info("\"Stage\":\"Before swap\",\"Old_PID\":%d,\"New_PID\":%d,"
				"\"old_pibr\":0x%llx,\"idx\":0x%x,\"incr\":0x%llx,"
				"\"size\":%d,\"ptr\":%p",
				pid, new_pid, curr_pibr, idx, incr, size, ptr);

		swap_and_flush_buffer(&ptr->pibr, new_pid);

		info("\"Stage\":\"After swap\",\"Old_PID\":%d,\"New_PID\":%d,"
				"\"old_pibr\":0x%llx,\"idx\":0x%x,\"incr\":0x%llx,"
				"\"size\":%d,\"ptr\":%p",
				pid, new_pid, curr_pibr, idx, incr, size, ptr);

#ifdef WRITE_TO_FILE
		if (new_pid != pid) {
			/* this must be done only once */
			int fd = __sync_fetch_and_or(&log_fd, 0);
			if (__sync_val_compare_and_swap(&log_fd, fd, -1) != -1)
				_close(fd);
		}
#endif
	};

	return size;
}

int bblog(const char* format, ...)
{
	static __thread char buffer[BUFFER_SIZE];

	ssize_t ret = snprintf(buffer, 32, "\"REC_%llu\":", cntr());
	size_t size = ret;

	info("\"Buffer\":%p,\"Size\":%d,\"Strlen(buffer)\":%d",
			buffer, size, strlen(buffer));

	va_list args;
	va_start(args, format);
	ret = vsnprintf(&buffer[size], (BUFFER_SIZE - size - 2), format, args);
	va_end(args);

	size += ret;
	if (size >= (BUFFER_SIZE - 2)) {
		err("Data too long %llu!", (uint64_t)size);
		return -1;
	}

	info("\"Buffer\":%p,\"Size\":%d,\"Strlen(buffer)\":%d",
			buffer, size, strlen(buffer));

	/* JSON formatting */
	buffer[size] = ',';
	size++;
	buffer[size] = '\0'; /* standalone, incase not batched */

	return _blog(buffer, size);
}

void bblog_init(int main)
{
	(void)main;

	// create new buffer
	swap_buffers(0, getpid());

#ifdef FLUSHER_THREAD
	/* start flusher thread */
	start_flusher_thread(&proc_pibr, &last_flush_secs);
#elif FLUSH_TIMER
	//start_flusher_timer(&proc_pibr);
#endif

	if (main) {
	}
}

/*
 * Parameter decoding:
 *
 * -tive @main means signal delivered
 * (signal num = abs val of @main),
 *
 * main = 1 means process extiting
 * main = 0 means thread exiting
 */
void bblog_exit(int main)
{
	(void)main;

	/* handle terminal signals */
	if (main < 0) {
		/* flush buffered messages */
		swap_and_flush_buffer(proc_pibr, getpid());
	}

	else if (!main) {
		// one of the threads just exited
		// do not flush/swap buffer here
	}

	/* XXX exec also calls this function */
	else if (main == 1) {
#ifdef FLUSHER_THREAD
		stop_flusher_thread();
#elif FLUSH_TIMER
		stop_flusher_timer();
#endif
		/* flush buffered messages */
		uint64_t pibr = __sync_fetch_and_and(proc_pibr, 0);
		if (pibr && PIBR_PTR(pibr)) {
			info("main thread exiting proc_pibr: 0x%llx main: %d",
				pibr, main);
			flush_buffer(PIBR_PTR(pibr));
		}
	}
}
#endif
