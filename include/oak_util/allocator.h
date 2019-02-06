#pragma once

#include <cstdlib>

#include "types.h"

namespace oak {

	namespace detail {

		inline void std_free_wrapper(void *ptr, size_t size) {
			std::free(ptr);
		}

	}

	inline void* (*alloc)(size_t size) = std::malloc;
	inline void (*free)(void *ptr, size_t size) = detail::std_free_wrapper;

}

void* operator new (std::size_t, void* p);

namespace oak::experimental {

	struct Allocator {
		void *(*allocFn)(u64 size, u64 alignment) = nullptr;
		void (*freeFn)(void *ptr, u64 size) = nullptr;

	};

	template<typename T>
	inline void mem_alloc(Allocator * const allocator, T ** const ptr, i64 const count) {
		*ptr = allocator->allocFn(sizeof(T) * count, alignof(T));
	}

	template<typename T, typename V>
	inline void mem_make(Allocator * const allocator, T ** const ptr, i64 const count, V&& value) {
		mem_alloc(allocator, ptr, count);
		for (i64 i = 0; i < count; ++i) {
			new ((*ptr) + i) T{ static_cast<V&&>(value) };
		}

	}

	template<typename T>
	inline void mem_free(Allocator *allocator, T *ptr, i64 count) {
		allocator->freeFn(ptr, sizeof(T) * count);
	}

}
