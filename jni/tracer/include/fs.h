#ifndef __STUDY_FS_FS_H__
#define __STUDY_FS_FS_H__

#include <log/logger.h>
#include <cutils/log.h>

#include "process.h"

#define LOG_ERROR_TAG "Storage-IOERR-UIUC"
#define ERR(fmt, ...)	\
	__android_log_print(ANDROID_LOG_ERROR, LOG_ERROR_TAG, fmt, ##__VA_ARGS__)

#define err(fmt, ...)	\
	ERR("{\"PROC\":\"%s\",\"PID\":%d,\"TID\":%d,\"Action\":\"Error\","	\
			"\"Msg\":{" fmt "},\"FUNC\":\"%s\",\"LINE\":%d}",			\
			process_name(), getpid(), gettid(), ##__VA_ARGS__, __func__, __LINE__)

#ifdef DEBUG_OPS
#define LOG_DEBUG_TAG "Storage-IODBG-UIUC"
#define INFO(fmt, ...)	\
	__android_log_print(ANDROID_LOG_DEBUG, LOG_DEBUG_TAG, fmt, ##__VA_ARGS__)

#define info(fmt, ...)	\
	INFO("{\"PROC\":\"%s\",\"PID\":%d,\"TID\":%d,\"Action\":\"Info\","	\
			"\"Msg\":{" fmt "},\"FUNC\":\"%s\",\"LINE\":%d}",			\
			process_name(), getpid(), gettid(), ##__VA_ARGS__, __func__, __LINE__)
#else
#define info(fmt, ...)
#endif

#ifdef TEST_OPS
#define LOG_TEST_TAG "Storage-IOTST-UIUC"
#define TEST(fmt, ...)	\
	__android_log_print(ANDROID_LOG_DEBUG, LOG_TEST_TAG, fmt, ##__VA_ARGS__)

#define test(fmt, ...)	\
	TEST("{\"PROC\":\"%s\",\"PID\":%d,\"TID\":%d,\"Action\":\"Test\","	\
			"\"Msg\":{" fmt "},\"FUNC\":\"%s\",\"LINE\":%d}",			\
			process_name(), getpid(), gettid(), ##__VA_ARGS__, __func__, __LINE__)
#else
#define test(fmt, ...)
#endif

#ifdef BUFFER_BATCHING
#define BUFFER_SIZE		(PATH_MAX + PATH_MAX)
#else
#define BUFFER_SIZE		(LOGGER_ENTRY_MAX_PAYLOAD - 64)
#endif

#ifdef PER_FILE_BATCHING
#define IOSTR_BUFSIZE	(BUFFER_SIZE - PATH_MAX)
#else
#define IOSTR_BUFSIZE	0
#endif

__attribute__((visibility("hidden"))) int _notify;
__attribute__((visibility("hidden"))) int notify(int, int, const char*);
__attribute__((visibility("hidden"))) int fd_to_path(int, char*);
#endif /* __STUDY_FS_FS_H__ */
