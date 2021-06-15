#ifndef __FANOTIFY_SYSCALL_LIB_H__
#define __FANOTIFY_SYSCALL_LIB_H__

#include <unistd.h>
#include <sys/syscall.h>

#undef __NR_fanotify_init
#undef __NR_fanotify_mark

#ifndef __NR_fanotify_init
#if defined(__ARM_EABI__)
#define __NR_fanotify_init 367
#define __NR_fanotify_mark 368
#elif defined(__i386__)
#define __NR_fanotify_init 338
#define __NR_fanotify_mark 339
#else
#error "Unsupported architecture"
#endif
#endif

#include <cutils/log.h>

// fanotify_init and fanotify_mark functions are syscalls.
// The user space bits are not part of bionic so we add them here
// as well as fanotify.h
int fanotify_init (unsigned int flags, unsigned int event_f_flags)
{
	int ret = syscall(__NR_fanotify_init, flags, event_f_flags);
	if (ret == -1)
		ALOGE("fanotify_init: %s\n", strerror(errno));
	return ret;
}

// Add, remove, or modify an fanotify mark on a filesystem object.
int fanotify_mark (int fanotify_fd, unsigned int flags,
                   uint64_t mask, int dfd, const char *pathname)
{
	int ret;

	ALOGD("fanotify_mark: mask = %llx pathname = %s\n", mask, pathname);

	// On 32 bits platforms we have to convert the 64 bits mask into
	// two 32 bits ints.
	if (sizeof(void *) == 4) {
		union {
			uint64_t _64;
			uint32_t _32[2];
		} _mask;
		_mask._64 = mask;
		ret = syscall(__NR_fanotify_mark, fanotify_fd, flags,
	  	   _mask._32[0], _mask._32[1], dfd, pathname);
	} else
		ret = syscall(__NR_fanotify_mark, fanotify_fd, flags, mask, dfd, pathname);

	if (ret == -1)
		ALOGE("fanotify_mark: %s\n", strerror(errno));
	return ret;
}
#endif /* __FANOTIFY_SYSCALL_LIB_H__ */
