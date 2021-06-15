/*
**
** Copyright 2007-2014, The Android Open Source Project
**
** This file is dual licensed.  It may be redistributed and/or modified
** under the terms of the Apache 2.0 License OR version 2 of the GNU
** General Public License.
*/

#ifndef _LIBS_ANODYNE_LOG_LOGGER_H
#define _LIBS_ANODYNE_LOG_LOGGER_H

#include <stdint.h>
#include <anodyne/log.h>
#include <anodyne/log_read.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The userspace structure for version 1 of the anodyne_logger_entry ABI.
 * This structure is returned to userspace by the kernel logger
 * driver unless an upgrade to a newer ABI version is requested.
 */
struct anodyne_logger_entry {
    uint16_t    len;    /* length of the payload */
    uint16_t    __pad;  /* no matter what, we get 2 bytes of padding */
    int32_t     pid;    /* generating process's pid */
    int32_t     tid;    /* generating process's tid */
    int32_t     sec;    /* seconds since Epoch */
    int32_t     nsec;   /* nanoseconds */
    char        msg[0]; /* the entry's payload */
} __attribute__((__packed__));

/*
 * The userspace structure for version 2 of the anodyne_logger_entry ABI.
 * This structure is returned to userspace if ioctl(LOGGER_SET_VERSION)
 * is called with version==2; or used with the user space log daemon.
 */
struct anodyne_logger_entry_v2 {
    uint16_t    len;       /* length of the payload */
    uint16_t    hdr_size;  /* sizeof(struct anodyne_logger_entry_v2) */
    int32_t     pid;       /* generating process's pid */
    int32_t     tid;       /* generating process's tid */
    int32_t     sec;       /* seconds since Epoch */
    int32_t     nsec;      /* nanoseconds */
    uint32_t    euid;      /* effective UID of logger */
    char        msg[0];    /* the entry's payload */
} __attribute__((__packed__));

struct anodyne_logger_entry_v3 {
    uint16_t    len;       /* length of the payload */
    uint16_t    hdr_size;  /* sizeof(struct anodyne_logger_entry_v3) */
    int32_t     pid;       /* generating process's pid */
    int32_t     tid;       /* generating process's tid */
    int32_t     sec;       /* seconds since Epoch */
    int32_t     nsec;      /* nanoseconds */
    uint32_t    lid;       /* log id of the payload */
    char        msg[0];    /* the entry's payload */
} __attribute__((__packed__));

/*
 * The maximum size of the log entry payload that can be
 * written to the logger. An attempt to write more than
 * this amount will result in a truncated log entry.
 */
#define ANODYNE_LOGGER_ENTRY_MAX_PAYLOAD	65516

/*
 * The maximum size of a log entry which can be read from the
 * kernel logger driver. An attempt to read less than this amount
 * may result in read() returning EINVAL.
 */
#define ANODYNE_LOGGER_ENTRY_MAX_LEN		(65*1024)

#define NS_PER_SEC 1000000000ULL

#if 0
struct log_msg {
    union {
        unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1];
        struct anodyne_logger_entry_v3 entry;
        struct anodyne_logger_entry_v3 entry_v3;
        struct anodyne_logger_entry_v2 entry_v2;
        struct anodyne_logger_entry    entry_v1;
    } __attribute__((aligned(4)));
#ifdef __cplusplus
    /* Matching log_time operators */
    bool operator== (const log_msg &T) const
    {
        return (entry.sec == T.entry.sec) && (entry.nsec == T.entry.nsec);
    }
    bool operator!= (const log_msg &T) const
    {
        return !(*this == T);
    }
    bool operator< (const log_msg &T) const
    {
        return (entry.sec < T.entry.sec)
            || ((entry.sec == T.entry.sec)
             && (entry.nsec < T.entry.nsec));
    }
    bool operator>= (const log_msg &T) const
    {
        return !(*this < T);
    }
    bool operator> (const log_msg &T) const
    {
        return (entry.sec > T.entry.sec)
            || ((entry.sec == T.entry.sec)
             && (entry.nsec > T.entry.nsec));
    }
    bool operator<= (const log_msg &T) const
    {
        return !(*this > T);
    }
    uint64_t nsec() const
    {
        return static_cast<uint64_t>(entry.sec) * NS_PER_SEC + entry.nsec;
    }

    /* packet methods */
    anodyne_log_id_t id()
    {
        return (anodyne_log_id_t) entry.lid;
    }
    char *msg()
    {
        return entry.hdr_size ? (char *) buf + entry.hdr_size : entry_v1.msg;
    }
    unsigned int len()
    {
        return (entry.hdr_size ? entry.hdr_size : sizeof(entry_v1)) + entry.len;
    }
#endif
};

struct logger;

anodyne_log_id_t android_anodyne_logger_get_id(struct logger *logger);
#endif

/*
 * anodyne_log_id_t helpers
 */
anodyne_log_id_t anodyne_name_to_log_id(const char *logName);
const char *anodyne_log_id_to_name(anodyne_log_id_t log_id);

#ifdef __cplusplus
}
#endif

extern "C"
int anodyne_log_write(int, const char *, const char *);

#endif /* _LIBS_ANODYNE_LOG_LOGGER_H */
