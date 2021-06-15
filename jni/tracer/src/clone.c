#define LOG_TAG "Storage-IOERR-UIUC"

#define _GNU_SOURCE 1
#include <sched.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cutils/log.h>

#include "ops.h"
#include "common.h"

static clone_t _clone;

static int clone_init(void)
{
	if (!_clone) {
		_clone = (clone_t) dlsym(RTLD_NEXT, "clone");
		if (!_clone)
    		_clone = (clone_t) dlsym(RTLD_NEXT, "__clone");
	}

	if (!_clone) {
		errno = EIO;
		ALOGE("NULL _clone!");
		return -1;
	}

	return 0;
}

int clone(int (*fn)(void*), void* child_stack, int flags, void* arg, ...)
{
	if (clone_init() == -1)
		return -1;

	pid_t *ptid = NULL;
	struct user_desc *tls = NULL;
	pid_t *ctid = NULL;

	// Extract any optional parameters required by the flags.
	va_list args;
	va_start(args, arg);
	if ((flags & (CLONE_PARENT_SETTID|CLONE_SETTLS|CLONE_CHILD_SETTID|CLONE_CHILD_CLEARTID)) != 0) {
	  ptid = va_arg(args, int*);
	}
	if ((flags & (CLONE_SETTLS|CLONE_CHILD_SETTID|CLONE_CHILD_CLEARTID)) != 0) {
	  tls = va_arg(args, void*);
	}
	if ((flags & (CLONE_CHILD_SETTID|CLONE_CHILD_CLEARTID)) != 0) {
	  ctid = va_arg(args, int*);
	}
	va_end(args);

	int ret = _clone(fn, child_stack, flags, arg, ptid, tls, ctid);
	if (ret != -1 && ret)
		pthread_common(1, 0);

	return ret;
}
