#ifdef FLUSHER_THREAD

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
#include <dlfcn.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "process.h"
#include "fs.h"
#include "ops.h"
#include "bblog.h"

#include <pthread.h>

/* flusher thread info */
struct thread_info {
	pthread_t tid;
	uint64_t **ptr;
	uint64_t *last_flush;
	int interrupt;
};

#define FLUSH_TIMEOUT 5 // in seconds

static pthread_create_t _pthread_create;
static int init_pthread(void)
{
	if (!_pthread_create) {
		_pthread_create = (pthread_create_t) dlsym(RTLD_NEXT,
													"pthread_create");
		if (!_pthread_create)
    		_pthread_create = (pthread_create_t) dlsym(RTLD_NEXT,
														"__pthread_create");
	}

	if (!_pthread_create) {
		errno = EIO;
		err("\"pthread\":0");
		return -1;
	}
	return 0;
}

#define FLUSH_BUF_SIZE	1024

static struct thread_info *flusher_info;

static void *flusher_thread(void *data)
{
	info("PID: %d TID: %d flusher thread started",
		getpid(), gettid());

	struct thread_info *info = (struct thread_info *)data;
	if (!info || !info->ptr)
		return NULL;

	err("PID: %d TID: %d flusher thread running",
		getpid(), gettid());

	uint16_t idx;
	pid_t pid, new_pid;
	uint64_t pibr,last_flush_secs;
	uint64_t **proc_pibr = info ? info->ptr : NULL;
	struct timespec ts;

	while (1) {

		/* state before sleeping */
		sleep(FLUSH_TIMEOUT);

		if (!strcmp(process_name(), "srg.cs.illinois.edu.storagestudy"))
			break;

		/* check termination request */
		if (__atomic_load_n (&info->interrupt, __ATOMIC_SEQ_CST))
			break;

		/*
		 * at this point we have established same state
		 * now, check for process inactivity
		 */
		clock_gettime(CLOCK_MONOTONIC, &ts);
		last_flush_secs = __atomic_load_n (info->last_flush, __ATOMIC_SEQ_CST); 
		if ((ts.tv_sec - last_flush_secs) <= FLUSH_TIMEOUT)
			continue;

		/* state after sleeping */
		if (!*proc_pibr)
			continue;

		/* check if there is something useful to flush */
		pibr = __atomic_load_n (*proc_pibr, __ATOMIC_SEQ_CST);
		if (!pibr || !PIBR_PTR(pibr))
			continue;

		/* check for pid match (avoid stale buffer flush) */
		pid = PIBR_PID(pibr);
		new_pid = getpid();
		if (pid != new_pid)
			continue;

		/* idx must be above threshold (avoid flushing of prefix only) */ 
		idx = PIBR_IDX(pibr);
		if (idx < FLUSH_BUF_SIZE)
			continue;

		info("flushing pid: %d tid: %d process: %s idx: 0x%x pibr: 0x%llx "
				"last flush time: %llu tv_sec: %llu idx: 0x%x",
				new_pid, gettid(), process_name(), idx, **proc_pibr,
				last_flush_secs, (uint64_t)ts.tv_sec, idx);

		swap_and_flush_buffer(*proc_pibr, new_pid);
	}

	err("PID: %d TID: %d flusher thread exiting", getpid(), gettid());
	free(info);
	return NULL;
}

static int system_xbin_fd = -1;
static int system_bin_fd = -1;
static int sbin_fd = -1;

static openat_t _openat;
static void override_defaults()
{
	if (!_openat) {
		_openat = (openat_t) dlsym(RTLD_NEXT, "openat");
		if (!_openat)
    		_openat = (openat_t) dlsym(RTLD_NEXT, "__open");
	}
}

static int in_dir(const char *dir_name, int *dir_fd, const char *proc)
{
	override_defaults();
	if (*dir_fd == -1)
		*dir_fd = _openat(AT_FDCWD, dir_name, O_RDONLY);	

	int ret;
	if (*dir_fd == -1 || *proc == '/') {
		ret = !strncmp(proc, dir_name, strlen(dir_name));
		info("proc: %s dir_name: %s, dir_fd: %d faccess: %d",
			proc, dir_name, *dir_fd, ret);
	} else {
		ret = !faccessat(*dir_fd, proc, F_OK, 0);
		info("proc: %s dir_name: %s, dir_fd: %d faccess: %d",
			proc, dir_name, *dir_fd, ret);
	}

	return ret;
}

static int skip_process(void)
{
	const char *proc = process_name();
	return !proc ||
			in_dir("/system/xbin/", &system_xbin_fd, proc) ||
			in_dir("/system/bin/", &system_bin_fd, proc) ||
			in_dir("/sbin", &sbin_fd, proc);
}

int start_flusher_thread(uint64_t **ptr, uint64_t *last_flush)
{
	if (!ptr || flusher_info || getppid() == 2 || skip_process())
		return 0;

	if (init_pthread() == -1) {
		err("\"Pthread init\":\"%s\"", strerror(errno));
		return -1;
	}

	struct thread_info *info = (struct thread_info *)
									malloc(sizeof(struct thread_info));
	if (!info) {
		err("\"Flusher malloc\":\"%s\"", strerror(errno));
		return -1;
	}

	if (!__sync_bool_compare_and_swap(&flusher_info, 0, info)) {
		free(info);
		return 0;
	}

	info->ptr = ptr;
	info->interrupt = 0;
	info->last_flush = last_flush;

	if (_pthread_create(&info->tid, NULL, flusher_thread,
						(void *)flusher_info) == -1) {
		err("\"Flusher create\":\"%s\"", strerror(errno));
		__sync_bool_compare_and_swap(&flusher_info, info, NULL);
		free(info);
		return -1;
	}

	info("PID: %d TID: %d flusher thread created", getpid(), gettid());
	return 0;
}

/* stop flusher thread */
void stop_flusher_thread(void)
{
	if (flusher_info) {
		pthread_t tid = flusher_info->tid;
		__sync_fetch_and_or(&flusher_info->interrupt, 1);
		pthread_join(tid, NULL);
		err("flusher thread exited");
		free(flusher_info);
	}
}
#endif

