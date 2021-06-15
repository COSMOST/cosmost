#ifndef __STUDY_FILEDB_CLIENT_H__
#define __STUDY_FILEDB_CLIENT_H__

#include <cutils/log.h>

class IFileDB;

// Client
class BpFileDB : public BpInterface<IFileDB> {
    public:
        BpFileDB(const sp<IBinder>& impl) : BpInterface<IFileDB>(impl) {
            ALOGD("BpFileDB::BpFileDB()");
        }

		virtual ~BpFileDB() {}

        virtual int32_t insert(const char *path) {
            Parcel data, reply;
			const String16 filePath(path);
            data.writeInterfaceToken(IFileDB::getInterfaceDescriptor());
            data.writeString16(filePath);
            remote()->transact(INSERT, data, &reply);

            int32_t res;
            status_t status = reply.readInt32(&res);
            ALOGD("BpFileDB::insert(%s) = %i (status: %i)", path, res, status);
            return res;
        }

        virtual int32_t remove(const char *path) {
            Parcel data, reply;
			const String16 filePath(path);
            data.writeInterfaceToken(IFileDB::getInterfaceDescriptor());
            data.writeString16(filePath);
            remote()->transact(REMOVE, data, &reply);

            int32_t res;
            status_t status = reply.readInt32(&res);
            ALOGD("BpFileDB::remove(%s) = %i (status: %i)", path, res, status);
            return res;
        }

        virtual int32_t update(const char *o, const char *n) {
            Parcel data, reply;
			const String16 oldPath(o);
			const String16 newPath(n);
            data.writeInterfaceToken(IFileDB::getInterfaceDescriptor());
            data.writeString16(oldPath);
            data.writeString16(newPath);
            remote()->transact(UPDATE, data, &reply);

            int32_t res;
            status_t status = reply.readInt32(&res);
            ALOGD("BpFileDB::update(%s, %s) = %i (status: %i)", o, n, res, status);
            return res;
        }
};

#if 0
sp<IFileDB> getFileDBService()
{
	sp<IServiceManager> sm = defaultServiceManager();
	ASSERT(sm != 0);
	sp<IBinder> binder = sm->getService(String16("FileDB"));
	// TODO: If the "FileDB" service is not running, getService times out and
	// binder == 0.
	ASSERT(binder != 0);
	sp<IFileDB> db = interface_cast<IFileDB>(binder);
	ASSERT(db != 0);
	return db;
}
#endif
#endif /* __STUDY_FILEDB_CLIENT_H__ */
