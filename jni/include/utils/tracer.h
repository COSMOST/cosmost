#ifndef __STUDY_UTILS_TRACER_H__
#define __STUDY_UTILS_TRACER_H__

#include <atomic>

#include <utils/list.h>
#include <utils/thread.h>
#include <utils/atomic.h>

namespace Utils {

	/*
	 *  --------------------------------------
	 * | Higher 32-bit Max | Lower 32-bit Ptr |
	 *  --------------------------------------
	 */
	#define TB_CURR_PTR_MASK 0xffffffffU
	#define TB_MAX_PTR_SHIFT 32
	typedef uint64_t tracebuf_t;

	/*
	 *  ------------------------------------------------
	 * | Higher 32-bit Head Ptr | Lower 32-bit TB Count |
	 *  ------------------------------------------------
	 */
	#define TBH_TB_COUNT_MASK 	0xffffffffU
	#define TBH_HEAD_SHIFT		32
	typedef uint64_t tracebuf_head_t;

	typedef struct {
		struct list_head entry;
		uintptr_t buffer;
	} tracebuf_node_t;

	#define FLUSH_TIMEOUT 150 // 5 min timeout

	template <class T, int size>
	class Tracer : public Utils::Thread {
		private:
			std::atomic<tracebuf_t> _active;	// active trace buffer
			std::atomic<tracebuf_head_t> _head; // queue of expired buffers
			std::atomic<uint64_t> _rec_count;	// trace record influx

			uint32_t _flush_timeout;
			const unsigned int max = (sizeof(T) * size);
			uint64_t _ts; // time (sec) last trace entry was made

		private:
			// trace buffer allocation
			uintptr_t alloc_trace_buffer() {
				T *ptr = new T[size];
				if (!ptr)
					throw Utils::Exception("Memory allocation failed!");

				memset(ptr, 0, max); // XXX perf penalty!
				INFO("tracebuf alloc: 0x%llx\n", (uint64_t)ptr);
				return reinterpret_cast<uintptr_t>(ptr);
			}

			void free_trace_buffer(uintptr_t buf) {
				T *ptr = reinterpret_cast<T *>(buf);
				delete [] ptr;
			}

			// trace buffer initialization
			tracebuf_t init_trace_buffer() {

				uintptr_t ptr = alloc_trace_buffer();
				tracebuf_t val = (uintptr_t)ptr + max;
				val <<= TB_MAX_PTR_SHIFT; // pack (max, ptr)
				val |= (uintptr_t)ptr;

				INFO("tracebuf: 0x%llx\n", val);
				return val;
			}

			// trace head allocation
			uintptr_t alloc_trace_head() {
				struct list_head *head = (struct list_head *)
											malloc(sizeof(struct list_head));
				if (!head)
					throw Utils::Exception("Memory allocation failed!");

				INIT_LIST_HEAD(head);
				INFO("head alloc: 0x%llx\n", (uint64_t)head);
				return reinterpret_cast<uintptr_t>(head);
			}

			// trace head initialization
			tracebuf_head_t init_trace_head() {

				uintptr_t head = alloc_trace_head();
				tracebuf_head_t val = head;
				val <<= TBH_HEAD_SHIFT; // pack (head, count)

				INFO("tracebuf_head: 0x%llx\n", val);
				return val;
			}

		public:
			Tracer() {

				// validate assumptions
				if (sizeof(uintptr_t) != (sizeof(uint64_t) >> 1))
					throw Utils::Exception("Pointer size mismatch!");

				// initialization
				_flush_timeout = FLUSH_TIMEOUT;
				_active = init_trace_buffer();
				_head = init_trace_head();

				// schedule flush thread
				start();
			}

			virtual ~Tracer() {
				interrupt();
				join();
			}

			/* FIXME */
			void adjustFlushTimeout(uint64_t elapsed) {
				// incoming rate is slowing down
				// exp backoff flushing
				_flush_timeout <<= 1;
			}

			/* flush thread */
			void run() {
				tracebuf_t buf;
				struct timespec ts;
				uintptr_t cptr, mptr;

				while (!interruptionRequested()) {
					sleep(_flush_timeout);

					// check for tracing inactivity
					clock_gettime(CLOCK_MONOTONIC, &ts);
					//INFO("flush thread time %llu secs\n", (uint64_t)ts.tv_sec);
					INFO("time elapsed: %llu\n",
							(uint64_t)(ts.tv_sec - atomic_read(&_ts)));

					if ((ts.tv_sec - atomic_read(&_ts)) > _flush_timeout) {

						// peek into current trace buffer
						buf = _active.fetch_add(0);
						cptr = buf & TB_CURR_PTR_MASK;
						mptr = buf >> TB_MAX_PTR_SHIFT;

						// check if there are enough floating records
						if (mptr - cptr > (max >> 1)) {
							tracebuf_t newbuf = init_trace_buffer(); // new buffer
							buf = _active.exchange(newbuf);
							enqueueTraceBuffer(cptr - max);
						}
					}

					// peek into header to check if there are enqueued buffers
					tracebuf_head_t tbh = _head.fetch_add(0);
					uint32_t count = tbh & TBH_TB_COUNT_MASK;

					INFO("flush thread retrieved HEAD: 0x%llx count: %u\n",
						(uint64_t)tbh, count);

					if (!count)
						continue;

					// create new header to swap
					tracebuf_head_t new_tbh = 0ULL;
					if (!interruptionRequested())
						new_tbh = init_trace_head(); // new head

					// swap headers now
					tbh = _head.exchange(new_tbh);
					// process enqueued buffers
					struct list_head *head =
									(struct list_head *)(tbh >> TBH_HEAD_SHIFT);

					tracebuf_node_t *node;
					for (node = list_entry((head)->next, tracebuf_node_t, entry);
	     				&node->entry != (head);
	     				node=list_entry(node->entry.next,tracebuf_node_t,entry)){
						flush((void *)node->buffer, max);
						free_trace_buffer(node->buffer);
						count--;
					}

					if (count)
						WARN("Sync error: %d trace buffers not flushed", count);

					free(head);
				}
			}

			void enqueueTraceBuffer(uintptr_t buffer)
			{
				tracebuf_node_t *node = (tracebuf_node_t *)
											malloc(sizeof(tracebuf_node_t));
				if (!node) {
					WARN("Failed to add trace buffer: mem alloc failed!");
					return;
				}

				INFO("allocated node: 0x%llx with buffer: 0x%llx\n",
						(uint64_t)node, (uint64_t)buffer);

				node->buffer = buffer;
				tracebuf_head_t tbh = _head.fetch_add(1UL);

				INFO("retrieved HEAD: 0x%llx\n", (uint64_t)tbh);

				struct list_head *head = (struct list_head *)
											(tbh >> TBH_HEAD_SHIFT);
				if (!head) {
					WARN("Invalid trace buffer header; ignoring enqueue op!\n");
					return;
				}

				list_add_tail(&node->entry, head);
				//INFO("added node to the list\n");
			}

			void addEntry(const T &rec)
			{
				// timekeeping
				struct timespec ts;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				atomic_write(&_ts, (uint64_t)ts.tv_sec);

				//INFO("time %llu secs\n", (uint64_t)_ts);

				tracebuf_t buf, newbuf;
				uintptr_t cptr, mptr;

				do {
					// atomic fetch
					buf = _active.fetch_add(sizeof(T));
					cptr = buf & TB_CURR_PTR_MASK;
					mptr = buf >> TB_MAX_PTR_SHIFT;

					// check if buffer full
					if (cptr == mptr) {
						newbuf = init_trace_buffer(); // new buffer
						buf = _active.exchange(newbuf);
						enqueueTraceBuffer(cptr - max);
						continue;
					}

					// continue recording
					else if (cptr < mptr) {
						memcpy((void *)cptr, &rec, sizeof(T));
						break;
					}
				} while(1);
			}

			virtual void flush(void *, size_t) = 0;
	};
}

#endif /* __STUDY_UTILS_TRACER_H__ */
