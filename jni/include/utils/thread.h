#ifndef __STUDY_UTILS_THREAD_H__
#define __STUDY_UTILS_THREAD_H__

#include <stdint.h>

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) \
    || defined(__NetBSD__)
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "OS not supported!"
#endif

#include <string>

namespace Utils {

	#if defined(__APPLE__)
	#include "mach/mach_types.h"
	typedef mach_port_t threadid_t;
	#elif defined(__linux__)
	typedef pid_t threadid_t;
	#elif defined(__FreeBSD__)
	#include <sys/thr.h>
	typedef long threadid_t;
	#elif defined(__NetBSD__)
	typedef uintptr_t threadid_t;
	#elif defined(_WIN32)
	typedef DWORD threadid_t;
	#else
	#error "OS not supported!"
	#endif
	
	
	class Thread
	{
	public:
		enum ThreadPriority {
		    IdlePriority,
		    LowPriority,
		    NormalPriority,
		    HighPriority,
		    InheritPriority
		};
	
		enum ThreadState { NotStarted, Stopped, Running, Finished };
	
	    const static threadid_t TID_NOBODY = 0;
	
	    Thread();
	    Thread(const std::string &name);
	    virtual ~Thread();
	    std::string getName();
	    void setName(const std::string &name);
	    ThreadPriority getPriority();
	    void setPriority(ThreadPriority p);
	    int start();
	    virtual void run() = 0;
	    int terminate();
	    bool wait(unsigned long time = 0xFFFFFFFFU);
		int join();
		ThreadState state();
	    void interrupt();
	    bool interruptionRequested();
	
	    static threadid_t getID();
	protected:
	    static void exit(void *retval);
	    void sleep(unsigned long secs);
	    void usleep(unsigned long usecs);
	    void yield();
	private:
	    bool interrupted;
	    ThreadState cstate;
	    std::string tname;
	#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) \
	    || defined(__NetBSD__)
	    pthread_t tid;
	    pthread_attr_t *attr;
	#elif defined(_WIN32)
	    HANDLE tid;
	#else
	#error "OS not supported!"
	#endif
	};
}
	
#endif /* __STUDY_UTILS_THREAD_H__ */

