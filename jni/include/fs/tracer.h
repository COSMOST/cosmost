#ifndef __STUDY_FS_TRACER_H__
#define __STUDY_FS_TRACER_H__

#include <utils/tracer.h>
#include <md5/md5.h>

namespace FS {

	#define NUM_READ_RECORDS	10
	#define TS_THRESHOLD		2 // 2s

	struct ReadRecord {
		uint8_t hash[MD5_DIGEST_LENGTH];	// file path hash
		uint64_t sec;						// timestamp (secs)
		uint64_t usec;						// timestamp (usecs)
		uint64_t blk;						// starting block number
		uint32_t count;						// number of blocks
	}__attribute__((aligned (8)));

	#define STUDY_TRACE_FILE_NAME	"trace"

	class StudyFSTracer : public Utils::Tracer<ReadRecord, NUM_READ_RECORDS> {
		private:
			int _fd;
			std::string _path;
			ReadRecord _last_rec;

		public:
			StudyFSTracer(const char *path) {}
			~StudyFSTracer() {}

			void record(const ReadRecord &rec) {}
			void flush(void *ptr, size_t sz) {}
	};
}

#endif /* __STUDY_FS_TRACER_H__ */
