#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>
#include <pthread.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "ops.h"
#include "notify.h"
#include "common.h"

void pthread_common(int created, int main)
{
	static uint32_t num_active_threads;
	if (created) {
		__sync_fetch_and_add(&num_active_threads, 1);
		//ALOGD("%s: starting %s thread (active: %d)",
		//		main ? "main" : "", procname, num_active_threads);

		notify_thread_init(main);

	} else {
		int num_threads = __sync_fetch_and_sub(&num_active_threads, 1);
		//ALOGD("%s: exiting %s thread (active: %d)",
		//		main ? "main" : "", process_name(), num_active_threads);

		/* close files */
		close_common(-1);

		notify_thread_exit(main);

		num_threads--;
		if (main && num_threads)
			ALOGD("unaccountable %d active threads at process exit", num_threads);
	}
}

static pthread_create_t _pthread_create;
static int pthread_create_init(void)
{
	if (!_pthread_create) {
		_pthread_create = (pthread_create_t) dlsym(RTLD_NEXT, "pthread_create");
		if (!_pthread_create)
    		_pthread_create = (pthread_create_t) dlsym(RTLD_NEXT, "__pthread_create");
	}

	if (!_pthread_create) {
		errno = EIO;
		ALOGE("NULL _pthread_init!");
		return -1;
	}

	return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
					void *(*start_routine)(void *), void *arg)
{
	if (pthread_create_init() == -1)
		return -1;

	int ret = _pthread_create(thread, attr, start_routine, arg);
	if (ret != -1)
		pthread_common(1, 0);

	return ret;
}

static pthread_exit_t _pthread_exit;
static int pthread_exit_init(void)
{
	if (!_pthread_exit) {
		_pthread_exit = (pthread_exit_t) dlsym(RTLD_NEXT, "pthread_exit");
		if (!_pthread_exit)
    		_pthread_exit = (pthread_exit_t) dlsym(RTLD_NEXT, "__pthread_exit");
	}

	if (!_pthread_exit) {
		errno = EIO;
		ALOGE("NULL _pthread_exit!");
		return -1;
	}

	return 0;
}

void pthread_exit(void *data)
{
	pthread_common(0, 0);

	if (pthread_exit_init() == -1)
		exit(1);

	_pthread_exit(data);
}

