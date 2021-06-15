#ifdef FLUSH_TIMER

#define LOG_TAG "Storage-IO-UIUC"

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <cutils/log.h>
#include <log/logger.h>

#include "process.h"
#include "timer.h"

#define TIMEOUT_SIGNAL (SIGRTMAX-10)
#define FLUSH_TIMEOUT 	90

static timer_t timeout_timer;
static int timeout_signal;

/* Return the number of seconds between before and after, (after - before).
 * This must be async-signal safe, so it cannot use difftime().
*/
static inline double timespec_diff(const struct timespec after, const struct timespec before)
{
    return (double)(after.tv_sec - before.tv_sec)
         + (double)(after.tv_nsec - before.tv_nsec) / 1000000000.0;
}

/* Add positive seconds to a timespec, nothing if seconds is negative.
 * This must be async-signal safe.
*/
static inline void timespec_add(struct timespec *const to, const double seconds)
{
    if (to && seconds > 0.0) {
        long  s = (long)seconds;
        long  ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

        /* Adjust for rounding errors. */
        if (ns < 0L)
            ns = 0L;
        else
        if (ns > 999999999L)
            ns = 999999999L;

        to->tv_sec += (time_t)s;
        to->tv_nsec += ns;

        if (to->tv_nsec >= 1000000000L) {
            to->tv_nsec -= 1000000000L;
            to->tv_sec++;
        }
    }
}

/* Set the timespec to the specified number of seconds, or zero if negative seconds.
*/
static inline void timespec_set(struct timespec *const to, const double seconds)
{
    if (to) {
        if (seconds > 0.0) {
            const long  s = (long)seconds;
            long       ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

            if (ns < 0L)
                ns = 0L;
            else
            if (ns > 999999999L)
                ns = 999999999L;

            to->tv_sec = (time_t)s;
            to->tv_nsec = ns;

        } else {
            to->tv_sec = (time_t)0;
            to->tv_nsec = 0L;
        }
    }
}

static void
timeout_signal_handler(int signum, siginfo_t *info, void *context)
{
	(void)context;
	(void)info;
	(void)signum;

    /* Not a timer signal? */
    if (!info || info->si_code != SI_TIMER) {
		err("\"Timer fired\":\"Code Mismatch\"");
        return;
	}

	if (timeout_signal != signum) {
		err("\"Timer fired\":\"Signal Mismatch\"");
		return;
	}

	if (&timeout_timer != info->si_value.sival_ptr) {
		err("\"Timer fired\":\"Timer Mismatch\"");
		return;
	}

	err("\"Timer fired\":0x%lx", (long)timeout_timer);
}

void stop_flusher_timer(void)
{
	if (!strncmp(process_name(), "/system/xbin/", strlen("/system/xbin/")) ||
		!strncmp(process_name(), "/system/bin/", strlen("/system/bin/")) ||
		getppid() == 2)
		return;

	if (!__sync_fetch_and_or(&timeout_signal, 0)) {
		err("\"Timer\":\"Doesn't  Exist\"");
		return;
	}

    struct sigaction  act;
    struct itimerspec arm;

    /* Ignore the timeout signals. */
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(timeout_signal, &act, NULL)) {
		err("\"Sigaction(%d)\":\"%s\"", timeout_signal, strerror(errno));
		return;
	}

    /* Disarm any current timeouts. */
    arm.it_value.tv_sec = 0;
    arm.it_value.tv_nsec = 0L;
    arm.it_interval.tv_sec = 0;
    arm.it_interval.tv_nsec = 0;

    if (timer_settime(timeout_timer, 0, &arm, NULL) == -1) {
		err("\"timer_settime\":\"%s\"", strerror(errno));
		return;
	}

    /* Destroy the timer itself. */
    if (timer_delete(timeout_timer))
		err("\"timer_delete\":\"%s\"", strerror(errno));
}

static int get_free_signal(void)
{
	int signum = TIMEOUT_SIGNAL;

	struct sigaction old;
	while (signum < SIGRTMAX) {

		/* retrieve old sig handler */
		if (sigaction(signum, NULL, &old) == -1) {
			err("\"OLD Sigaction(%d)\":\"%s\"",
				signum, strerror(errno));
			signum++;
			continue;
		}

		if (!old.sa_handler)
			return signum;

		err("\"Signal %d\": \"Already Taken\"", signum);
		signum++;
	}

	err("\"SIGRTMAX reached\":%d", signum);
	return -1;
}

int start_flusher_timer(uint64_t *ptr)
{
	if (!strncmp(process_name(), "/system/xbin/", strlen("/system/xbin/")) ||
		!strncmp(process_name(), "/system/bin/", strlen("/system/bin/")) ||
		getppid() == 2)
		return 0;

#if 0
	if (!strcmp(process_name(), "/system/bin/servicemanager") ||
		!strcmp(process_name(), "/system/bin/subsystem_ramdump") ||
		!strcmp(process_name(), "/system/bin/logd") ||
		!strcmp(process_name(), "/system/bin/qseecomd"))
		return 0;
#endif

	(void)ptr;
	if (__sync_fetch_and_or(&timeout_signal, 0)) {
		//err("\"Timer\":\"Already Exists\"");
		return 0;
	}

	int signum = get_free_signal();
	if (signum == -1)
		return -1;

	if (!__sync_bool_compare_and_swap(&timeout_signal, 0, signum))
		return 0;

    /* Install timeout_signal_handler. */
    sigset_t mask;
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = timeout_signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(timeout_signal, &sa, NULL)) {
		err("\"Sigaction(SIGRTMIN)\":\"%s\"", strerror(errno));
		return -1;
	}

    /* Create a timer that will signal to timeout_signal_handler. */
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = timeout_signal;
    sev.sigev_value.sival_ptr = &timeout_timer;

    if (timer_create(CLOCK_REALTIME, &sev, &timeout_timer)) {
		err("\"timer_settime\":\"%s\"", strerror(errno));
		return -1;
	}

    /* Start the timer */
    struct itimerspec arm;
    arm.it_value.tv_sec = FLUSH_TIMEOUT;
    arm.it_value.tv_nsec = 0;
    arm.it_interval.tv_sec = FLUSH_TIMEOUT;
    arm.it_interval.tv_nsec = 0;

	if (timer_settime(timeout_timer, 0, &arm, NULL)) {
    	err("\"timer_settime\":\"%s\"", strerror(errno));
		return -1;
	}

    ALOGE("PID: %d TID: %d process_name: %s timer 0x%lx created\n",
			getpid(), gettid(), process_name(), (long) timeout_timer);

	return 0;
}
#endif
