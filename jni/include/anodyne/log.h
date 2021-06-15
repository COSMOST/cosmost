/*
 * Copyright (C) 2005-2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_ANODYNE_LOG_LOG_H
#define _LIBS_ANODYNE_LOG_LOG_H

#include <sys/types.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum anodyne_log_id {
    ANODYNE_LOG_ID_MIN = 0,

    ANODYNE_LOG_ID_MAIN = 0,
    ANODYNE_LOG_ID_RADIO = 1,
    ANODYNE_LOG_ID_EVENTS = 2,
    ANODYNE_LOG_ID_SYSTEM = 3,
    ANODYNE_LOG_ID_CRASH = 4,

    ANODYNE_LOG_ID_MAX
} anodyne_log_id_t;
#define sizeof_anodyne_log_id_t sizeof(typeof_anodyne_log_id_t)
#define typeof_anodyne_log_id_t unsigned char

#ifdef __cplusplus
}
#endif

#endif /* _LIBS_ANODYNE_LOG_LOG_H */
