#define LOG_TAG "Storage-IOERR-UIUC"

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

#include "process.h"
#include "fs.h"

#define alloca(size)   __builtin_alloca(size)
extern char **environ;

typedef int (*execve_t)(const char *, char *const [],
                  char *const []);

typedef int (*execvpe_t)(const char *, char *const *,
							char *const *);

static execve_t _execve;
static execvpe_t _execvpe;

int init_execve(void)
{
	if (!_execve) {
		_execve = (execve_t) dlsym(RTLD_NEXT, "execve");
		if (!_execve)
    		_execve = (execve_t) dlsym(RTLD_NEXT, "__execve");
	}

	if (!_execve) {
		errno = EIO;
		ALOGE("NULL _execve!");
		return -1;
	}

	return 0;
}

int execve(const char *name, char *const argv[],
                  char *const envp[])
{
	if (init_execve() == -1)
		return -1;

	err("exec: %s", name);
	(void)_execve(name, argv, envp);
	return -1;
}

int execl(const char *name, const char *arg, ...)
{
    va_list ap;
    char **argv;
    int n;

    va_start(ap, arg);
    n = 1;
    while (va_arg(ap, char *) != NULL)
        n++;
    va_end(ap);
    argv = alloca((n + 1) * sizeof(*argv));
    if (argv == NULL) {
        errno = ENOMEM;
        return (-1);
    }
    va_start(ap, arg);
    n = 1;
    argv[0] = (char *)arg;
    while ((argv[n] = va_arg(ap, char *)) != NULL)
        n++;
    va_end(ap);
    return (execve(name, argv, environ));
}

int
execle(const char *name, const char *arg, ...)
{
    va_list ap;
    char **argv, **envp;
    int n;

    va_start(ap, arg);
    n = 1;
    while (va_arg(ap, char *) != NULL)
        n++;
    va_end(ap);
    argv = alloca((n + 1) * sizeof(*argv));
    if (argv == NULL) {
        errno = ENOMEM;
        return (-1);
    }
    va_start(ap, arg);
    n = 1;
    argv[0] = (char *)arg;
    while ((argv[n] = va_arg(ap, char *)) != NULL)
        n++;
    envp = va_arg(ap, char **);
    va_end(ap);
    return (execve(name, argv, envp));
}

int
execlp(const char *name, const char *arg, ...)
{
    va_list ap;
    char **argv;
    int n;

    va_start(ap, arg);
    n = 1;
    while (va_arg(ap, char *) != NULL)
        n++;
    va_end(ap);
    argv = alloca((n + 1) * sizeof(*argv));
    if (argv == NULL) {
        errno = ENOMEM;
        return (-1);
    }
    va_start(ap, arg);
    n = 1;
    argv[0] = (char *)arg;
    while ((argv[n] = va_arg(ap, char *)) != NULL)
        n++;
    va_end(ap);
    return (execvp(name, argv));
}

int execv(const char *name, char *const *argv)
{
    (void)execve(name, argv, environ);
    return (-1);
}

int init_execvpe(void)
{
	if (!_execvpe) {
		_execvpe = (execvpe_t) dlsym(RTLD_NEXT, "execvpe");
		if (!_execvpe)
    		_execvpe = (execvpe_t) dlsym(RTLD_NEXT, "__execvpe");
	}

	if (!_execvpe) {
		errno = EIO;
		ALOGE("NULL _execvpe!");
		return -1;
	}

	return 0;
}

int execvpe(const char *name, char *const *argv, char *const *envp)
{
	if (init_execvpe() == -1)
		return -1;

	err("exec: %s", name);
	return _execvpe(name, argv, envp);
}

int execvp(const char *name, char *const *argv)
{
    return execvpe(name, argv, environ);
}
