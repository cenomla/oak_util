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
#endif

#include "types.h"

namespace oak {

	inline i32 atomic_load(i32 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile long*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
#endif // _MSC_VER
	}

	inline i64 atomic_load(i64 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile __int64*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
#endif // _MSC_VER
	}

	inline u32 atomic_load(u32 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile unsigned long*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
#endif // _MSC_VER
	}

	inline u64 atomic_load(u64 *mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<volatile unsigned __int64*>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
#endif // _MSC_VER
	}

	inline void* atomic_load(void **mem) noexcept {
#ifdef _MSC_VER
		_ReadWriteBarrier();
		return *reinterpret_cast<void * volatile *>(mem);
#else
		return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
#endif // _MSC_VER
	}

	inline i32 atomic_store(i32 *mem, i32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline i64 atomic_store(i64 *mem, i64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline u32 atomic_store(u32 *mem, u32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline u64 atomic_store(u64 *mem, u64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline void* atomic_store(void **mem, void* value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangePointer(reinterpret_cast<void * volatile *>(mem), value);
#else
		return __atomic_exchange_n(mem, value, __ATOMIC_ACQ_REL);
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
				__ATOMIC_ACQ_REL,
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
				__ATOMIC_ACQ_REL,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline bool atomic_compare_exchange(u64 *mem, u64 *expected, u64 value) noexcept {
#ifdef _MSC_VER
		auto prev = static_cast<u64>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(mem), value, *expected));
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
				__ATOMIC_ACQ_REL,
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
				__ATOMIC_ACQ_REL,
				__ATOMIC_RELAXED);
#endif // _MSC_VER
	}

	inline i32 atomic_fetch_add(i32 *mem, i32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline u32 atomic_fetch_add(u32 *mem, u32 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline i64 atomic_fetch_add(i64 *mem, i64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline u64 atomic_fetch_add(u64 *mem, u64 value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(mem), value);
#else
		return __atomic_fetch_add(mem, value, __ATOMIC_ACQ_REL);
#endif // _MSC_VER
	}

	inline void* atomic_fetch_add(void **mem, void *value) noexcept {
#ifdef _MSC_VER
		return _InterlockedExchangePointer(reinterpret_cast<void * volatile *>(mem), value);
#else
		return __atomic_fetch_add(mem, reinterpret_cast<long>(value), __ATOMIC_ACQ_REL);
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
		[[maybe_unused]] auto oldValue = atomic_store(lock, 0);
		assert(oldValue == 1 && "unlocked non locked lock");
	}

	inline void atomic_rw_lock_read(i32 *rwLock) noexcept {
		i32 locked = atomic_load(rwLock);
		if (locked > 0)
			locked = 0;

		while (!atomic_compare_exchange(rwLock, &locked, locked - 1))
			if (locked > 0)
				locked = 0;
	}

	inline bool atomic_rw_try_lock_read(i32 *rwLock) noexcept {
		i32 locked = atomic_load(rwLock);
		if (locked > 0)
			return false;

		while (!atomic_compare_exchange(rwLock, &locked, locked - 1))
			if (locked > 0)
				return false;

		return true;
	}

	inline void atomic_rw_unlock_read(i32 *rwLock) noexcept {
		[[maybe_unused]] auto oldValue = atomic_fetch_add(rwLock, 1);
		assert(oldValue < 0 && "unlocked non read locked rw lock");
	}

	inline void atomic_rw_lock_write(i32 *rwLock) noexcept {
		atomic_lock(rwLock);
	}

	inline i32 atomic_rw_try_lock_write(i32 *rwLock) noexcept {
		i32 locked = 0;
		if (atomic_compare_exchange(rwLock, &locked, 1))
			return 0;

		return locked;
	}

	inline void atomic_rw_unlock_write(i32 *rwLock) noexcept {
		atomic_unlock(rwLock);
	}
}

