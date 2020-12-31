#pragma once

#include "types.h"

namespace oak {

	enum class MemoryOrder {
		RELAXED, // No memory fence is used
		CONSUME, // Probably not used, idk what the hell this means, I think on x86 it's the same as acquire
		ACQUIRE, // Read memory fence with atomic op
		RELEASE, // Write memory fence with atomic op
		ACQUIRE_RELEASE, // Read and write memory fence with atomic op
		SEQUENTIAL_CONSISTENT, // Single total order for reads and writes on all threads, this is often more expensive than locks
	};

	namespace detail {
#ifdef _MSC_VER
#else
		constexpr int gcc_memory_order_constants[] = {
			__ATOMIC_RELAXED,
			__ATOMIC_CONSUME,
			__ATOMIC_ACQUIRE,
			__ATOMIC_RELEASE,
			__ATOMIC_ACQ_REL,
			__ATOMIC_SEQ_CST,
		};
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_load(T *mem, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_load_n(mem, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T, typename U>
	void atomic_store(T *mem, U value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		__atomic_store_n(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_exchange(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_exchange_n(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	bool atomic_compare_exchange_strong(T *mem, T *expected, T value, MemoryOrder successOrder, MemoryOrder failOrder) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				false,
				detail::gcc_memory_order_constants[enum_int(successOrder)],
				detail::gcc_memory_order_constants[enum_int(failOrder)]
				);
#endif // _MSC_VER
	}

	template<typename T>
	bool atomic_compare_exchange_weak(T *mem, T *expected, T value, MemoryOrder successOrder, MemoryOrder failOrder) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_compare_exchange_n(
				mem,
				expected,
				value,
				true,
				detail::gcc_memory_order_constants[enum_int(successOrder)],
				detail::gcc_memory_order_constants[enum_int(failOrder)]
				);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_add(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_add(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_sub(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_sub(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_and(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_and(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_xor(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_xor(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_or(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_or(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	template<typename T>
	T atomic_fetch_nand(T *mem, T value, MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		return __atomic_fetch_nand(mem, value, detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	inline void atomic_fence(MemoryOrder order) noexcept {
#ifdef _MSC_VER
		static_assert("Not implemented on msvc");
#else
		__atomic_thread_fence(detail::gcc_memory_order_constants[enum_int(order)]);
#endif // _MSC_VER
	}

	// NOTE: If needed later I'll add the op_fetch variants of the atomic_fetch_op functions

	inline void atomic_lock(i32 *lock) noexcept {
		i32 locked = 0;

		// TODO: Maybe use _mm_pause here to decrease cpu usage
		while (!atomic_compare_exchange_weak(lock, &locked, 1, MemoryOrder::ACQUIRE, MemoryOrder::RELAXED))
			locked = 0;
	}

	inline bool atomic_try_lock(i32 *lock) noexcept {
		i32 locked = 0;
		return atomic_compare_exchange_weak(lock, &locked, 1, MemoryOrder::ACQUIRE, MemoryOrder::RELAXED);
	}

	inline void atomic_unlock(i32 *lock) noexcept {
		atomic_store(lock, 0, MemoryOrder::RELEASE);
	}

}

