#ifndef __STUDY_LOG_ANDROID_H__
#define __STUDY_LOG_ANDROID_H__

#ifdef __ANDROID__

#include <jni.h>
#include <android/log.h>

#define LOG_TAG		"[STUDY]"
#define LOGI(...)	__android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...)	__android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...)	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif /* __ANDROID__ */

#endif /* __STUDY_LOG_ANDROID_H__ */
