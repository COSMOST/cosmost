#define LOG_TAG "Storage-IO-UIUC"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>

#include <cutils/log.h>
#include <log/logger.h>

#include "notify.h"
#include "fs.h"

/* other signal handlers */
static void (*old_sigint_handler)(int) = NULL;
static void (*old_sigterm_handler)(int) = NULL;
static void (*old_sigabrt_handler)(int) = NULL;
static void (*old_sigquit_handler)(int) = NULL;
static void (*old_sighup_handler)(int) = NULL;
static void (*old_sigtstp_handler)(int) = NULL;

static void aggregate_handler(int signum)
{
	if (signum == SIGABRT || signum == SIGINT ||
		signum == SIGHUP || signum == SIGTERM ||
		signum == SIGQUIT || signum == SIGTSTP) {
		err("\"Signal\":\"%s\"", strsignal(signum));
		notify_thread_exit(-signum);
	}

    if (signum == SIGINT && old_sigint_handler)
        old_sigint_handler(signum);

    else if (signum == SIGHUP && old_sighup_handler)
        old_sighup_handler(signum);

    else if (signum == SIGTSTP && old_sigtstp_handler)
        old_sigtstp_handler(signum);

    else if (signum == SIGTERM && old_sigterm_handler)
        old_sigterm_handler(signum);

    else if (signum == SIGQUIT && old_sigquit_handler)
        old_sigquit_handler(signum);
   
	 else if (signum == SIGABRT && old_sigabrt_handler)
        old_sigabrt_handler(signum);
}

void init_signal(void)
{
	struct sigaction sa;
	struct sigaction old;
	
	/* retrieve old sig handler */
	if (sigaction(SIGINT, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGINT)\":\"%s\"", strerror(errno));
	else
		old_sigint_handler = old.sa_handler;
	
	if (sigaction(SIGHUP, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGHUP)\":\"%s\"", strerror(errno));
	else
		old_sighup_handler = old.sa_handler;

	if (sigaction(SIGABRT, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGABRT)\":\"%s\"", strerror(errno));
	else
		old_sigabrt_handler = old.sa_handler;

	if (sigaction(SIGTERM, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGTERM)\":\"%s\"", strerror(errno));
	else
		old_sigterm_handler = old.sa_handler;

	if (sigaction(SIGQUIT, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGQUIT)\":\"%s\"", strerror(errno));
	else
		old_sigquit_handler = old.sa_handler;

	if (sigaction(SIGTSTP, NULL, &old) == -1)
		err("\"OLD Sigaction(SIGTSTP)\":\"%s\"", strerror(errno));
	else
		old_sigtstp_handler = old.sa_handler;

	/* set our own sig handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = aggregate_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1)
		err("\"Sigaction(SIGINT)\":\"%s\"", strerror(errno));

	if (sigaction(SIGABRT, &sa, NULL) == 1)
		err("\"Sigaction(SIGABRT)\":\"%s\"", strerror(errno));

	if (sigaction(SIGTERM, &sa, NULL) == -1)
		err("\"Sigaction(SIGTERM)\":\"%s\"", strerror(errno));

	if (sigaction(SIGHUP, &sa, NULL) == -1)
		err("\"Sigaction(SIGHUP)\":\"%s\"", strerror(errno));

	if (sigaction(SIGTSTP, &sa, NULL) == -1)
		err("\"Sigaction(SIGTSTP)\":\"%s\"", strerror(errno));

	if (sigaction(SIGQUIT, &sa, NULL) == -1)
		err("\"Sigaction(SIGQUIT)\":\"%s\"", strerror(errno));
}
