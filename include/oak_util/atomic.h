#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedExchangeAddPointer)
#endif

#include "types.h"

namespace oak {

	inline i32 atomic_load(i32 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile long*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline i64 atomic_load(i64 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile __int64*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline u64 atomic_load(u64 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile unsigned __int64*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline void* atomic_load(void **mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<void * volatile *>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline i32 atomic_store(i32 *mem, i32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline i64 atomic_store(i64 *mem, i64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline u64 atomic_store(u64 *mem, u64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline void* atomic_store(void **mem, void* value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangePointer(reinterpret_cast<void * volatile *>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline bool atomic_compare_exchange(i32 *mem, i32 *expected, i32 value) noexcept {
#ifdef _MSC_VER
		auto prev = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(mem), value, *expected);
		if (prev == *expected)
			return true;

		*expected = prev;
		return false;
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				false,
				__ATOMIC_ACQUIRE,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline bool atomic_compare_exchange(i64 *mem, i64 *expected, i64 value) noexcept {
#ifdef _MSC_VER
		auto prev = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(mem), value, *expected);
		if (prev == *expected)
			return true;

		*expected = prev;
		return false;
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				false,
				__ATOMIC_ACQUIRE,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline bool atomic_compare_exchange(u64 *mem, u64 *expected, u64 value) noexcept {
#ifdef _MSC_VER
		auto prev = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(mem), value, *expected);
		if (prev == *expected)
			return true;

		*expected = prev;
		return false;
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				false,
				__ATOMIC_ACQUIRE,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline bool atomic_compare_exchange(void **mem, void **expected, void *value) noexcept {
#ifdef _MSC_VER
		auto prev = _InterlockedCompareExchangePointer(reinterpret_cast<void * volatile *>(mem), value, *expected);
		if (prev == *expected)
			return true;

		*expected = prev;
		return false;
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				false,
				__ATOMIC_ACQUIRE,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline i32 atomic_fetch_add(i32 *mem, i32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline i64 atomic_fetch_add(i64 *mem, i64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline u64 atomic_fetch_add(u64 *mem, u64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline void* atomic_fetch_add(void **mem, void *value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangePointer(reinterpret_cast<void * volatile *>(mem), value);
#else
		return __atomic_fetch_add(mem, reinterpret_cast<long>(value), __ATOMIC_SEQ_CST);
#endif // _MSC_VER
	}

	inline void atomic_lock(i32 *lock) noexcept {
		i32 locked = 0;

		// TODO: Maybe use _mm_pause here to decrease cpu usage
		while (!atomic_compare_exchange(lock, &locked, 1))
			locked = 0;
	}

	inline bool atomic_try_lock(i32 *lock) noexcept {
		i32 locked = 0;
		return atomic_compare_exchange(lock, &locked, 1);
	}

	inline void atomic_unlock(i32 *lock) noexcept {
		atomic_store(lock, 0);
	}

}

