#ifndef __STUDY_LOG_H__
#define __STUDY_LOG_H__

#ifndef __ANDROID__
#include <stdio.h>
#define INFO(...)										\
	do {												\
		printf("[INFO] %s:%d ", __func__, __LINE__);	\
		printf(__VA_ARGS__);							\
	} while(0)

#ifdef DEBUG_MSGS
#define DEBUG(...)										\
	do {												\
		printf("[DEBUG] %s:%d ", __func__, __LINE__);	\
		printf(__VA_ARGS__);							\
	} while(0)
#else
#define DEBUG(...)
#endif

#define WARN(...)										\
	do {												\
		printf("[WARN] %s:%d ", __func__, __LINE__);	\
		printf(__VA_ARGS__);							\
	} while(0)

#define ERROR(...)	fprintf(stderr, __VA_ARGS__)

#else /* !__ANDROID__ */

#include <utils/android.h>

#ifdef DETAILED_MSGS

#define INFO(fmt, ...)	LOGI("%s:%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define WARN(fmt, ...)	LOGW("%s:%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define ERROR(fmt, ...)	LOGE("%s:%d " fmt, __func__, __LINE__, ##__VA_ARGS__)

#ifdef DEBUG_MSGS
#define DEBUG(fmt, ...)	LOGD("%s:%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#else /* !DETAILED_MSGS */

#define INFO	LOGI
#define WARN	LOGW
#define ERROR	LOGE

#ifdef DEBUG_MSGS
#define DEBUG	LOGD
#else
#define DEBUG(...)
#endif /* DEBUG_MSGS */

#endif /* DETAILED_MSGS */

#endif /* __ANDROID__ */

#endif /* __STUDY_LOG_H__ */
