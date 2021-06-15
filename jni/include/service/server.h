#ifndef __STUDY_FILEDB_SERVER_H__
#define __STUDY_FILEDB_SERVER_H__

#include <service/interface.h>

namespace android {

	// Server
	class FileDB : public BnFileDB {
		private:
			sp<MemoryHeapBase> mAndroidSharedMemory;
	
		public:
			FileDB();
			virtual ~FileDB();
	
	    	virtual int insert(const char *p);
	    	virtual int remove(const char *p);
	    	virtual int update(const char *o, const char *n);
			//virtual sp<IMemoryHeap> test3(const String8& message);
	};
}

#endif /* __STUDY_FILEDB_SERVER_H__ */
