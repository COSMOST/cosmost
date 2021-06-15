LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := liblogfileops

LOCAL_SRC_FILES := \
	src/bblog.c		\
	src/blog.c		\
	src/batch.c		\
	src/counter.c	\
	src/notify.c	\
	src/rename.c	\
	src/write.c		\
	src/fwrite.c	\
	src/pwrite.c	\
	src/unlink.c	\
	src/fopen.c		\
	src/close.c		\
	src/open.c		\
	src/read.c		\
	src/pread.c		\
	src/fread.c		\
	src/pthread.c	\
	src/process.c	\
	src/signal.c	\
	src/clone.c		\
	src/flusher.c	\
	src/timer.c		\
	src/mmap.c		\
	src/fs.c
#	src/exec.c		\

LOCAL_LDLIBS += -ldl -llog
LOCAL_SHARED_LIBRARIES := libutils libcutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../include

#-Wall -Werror \

LOCAL_CFLAGS := \
				-DHAVE_SYS_UIO_H \
				-DPER_FILE_BATCHING \
				-DBATCH_BLOCKS	\
				-DBUFFER_BATCHING \
				-DFLUSHER_THREAD \
				#-DFLUSH_TIMER \
				#-DTEST_OPS \
				#-DDEBUG_OPS \
				#-DWRITE_TO_FILE \
				-DTEST_BUFFER_BATCHING \
				-DTEST_FILE_BATCHING \

include $(BUILD_SHARED_LIBRARY)
