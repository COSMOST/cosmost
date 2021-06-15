#ifndef __STUDY_UTILS_ATOMIC_H__
#define __STUDY_UTILS_ATOMIC_H__

#include <utils/exception.h>

namespace Utils {

	template<typename T>
	static inline
	T atomic_read(volatile T *addr)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		return __sync_fetch_and_or(addr, static_cast<T>(0));
	}
	
	template<typename T>
	static inline
	T atomic_fetch_and_or(volatile T *addr, T val)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned!");
		return __sync_fetch_and_or(addr, val);
	}
	
	template<typename T>
	static inline
	T atomic_fetch_and_and(volatile T *addr, T val)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		return __sync_fetch_and_and(addr, val);
	}
	
	template<typename T>
	static inline
	T atomic_fetch_and_add(volatile T *addr, T val)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		return __sync_fetch_and_add(addr, val);
	}
	
	template<typename T>
	static inline
	T atomic_fetch_and_sub(volatile T *addr, T val)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		return __sync_fetch_and_sub(addr, val);
	}
	
	template<typename T>
	static inline
	void atomic_write(volatile T *addr, T val)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		(void)__sync_lock_test_and_set(addr, val);
	}

	template<typename T>
	static inline
	T atomic_compare_swap(volatile T *addr, T o, T n)
	{
		if ((uintptr_t)addr & (sizeof(T) - 1))
			throw Exception("must be aligned");
		return __sync_val_compare_and_swap(addr, o, n);
	}
}
#endif /* __STUDY_UTILS_ATOMIC_H__ */
