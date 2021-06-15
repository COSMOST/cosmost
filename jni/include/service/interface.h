#ifndef __STUDY_IFILEDB_SERVICE_H__
#define __STUDY_IFILEDB_SERVICE_H__

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <binder/IInterface.h>
#include <binder/MemoryHeapBase.h>

namespace android {

	// Interface (our AIDL) - Shared by server and client
	class IFileDB : public IInterface {
	    public:
	        enum {
	            INSERT = IBinder::FIRST_CALL_TRANSACTION,
	            REMOVE,
	            UPDATE
	        };
	
			DECLARE_META_INTERFACE(FileDB);
	
	        virtual int insert(const char *) = 0;
	        virtual int remove(const char *) = 0;
	        virtual int update(const char *, const char *) = 0;
	
			//virtual sp<IMemoryHeap> test3(const String8& message) = 0;
	};
	
	// Server
	class BnFileDB : public BnInterface<IFileDB> {
		public:
			BnFileDB();
			//virtual ~BnFileDB();
	
			virtual status_t onTransact(uint32_t code, const Parcel& data,
										Parcel* reply, uint32_t flags = 0);
	};
}
#endif /* __STUDY_IFILEDB_SERVICE_H__ */
