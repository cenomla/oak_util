#pragma once

#include <cstdlib>
#include <new>

#include "types.h"

namespace oak {

	struct MemoryArenaHeader {
		int64_t allocationCount;
		int64_t requestedMemory;
		int64_t usedMemory;
		void *next;
	};

	struct StackHeader {
		int64_t allocationCount;
		int64_t requestedMemory;
	};

	struct PoolNode {
		PoolNode *next;
		size_t size;
	};

	struct PoolHeader {
		PoolNode *freeList;
		size_t poolSize;
	};

	struct MemoryArena {
		void *block = nullptr;
		size_t size = 0;
	};

	namespace detail {

		inline void std_free_wrapper(void *ptr, size_t) {
			std::free(ptr);
		}

	}

	Result init_memory_arena(MemoryArena *arena, size_t size);
	Result init_memory_arena(MemoryArena *arena, void *block, size_t size);

	void* allocate_from_arena(MemoryArena *arena, size_t size, int64_t count, size_t alignment);
	void copy_arena(MemoryArena *dst, MemoryArena *src);
	void clear_arena(MemoryArena *arena);
	bool arena_contains(MemoryArena *arena, void *ptr);

	void* push_stack(MemoryArena *arena);
	void pop_stack(MemoryArena *arena, void *stackPtr);

	MemoryArena create_pool(MemoryArena *arena, size_t size);
	void* allocate_from_pool(MemoryArena *arena, size_t size, int64_t count);
	void free_from_pool(MemoryArena *arena, const void *ptr, size_t size, int64_t count);

	struct ArenaStack {
		MemoryArena *arena = nullptr;
		void *stackPtr = nullptr;

		ArenaStack(MemoryArena *arena_)
			: arena{ arena_ }, stackPtr{ push_stack(arena) } {}

		~ArenaStack() {
			pop_stack(arena, stackPtr);
		}
	};

	template<typename T>
	T* allocate(MemoryArena *arena, int64_t count) {
		return static_cast<T*>(allocate_from_arena(arena, sizeof(T), count, alignof(T)));
	}

	template<typename T, typename... TArgs>
	T* make(MemoryArena *arena, int64_t count, TArgs&&... parameters) {
		auto result = static_cast<T*>(allocate_from_arena(arena, sizeof(T), count, alignof(T)));
		if (result) {
			for (int64_t i = 0; i < count; i++) {
				new (result + i) T{ static_cast<TArgs&&>(parameters)... };
			}
		}
		return result;
	}

	inline void* (*alloc)(size_t size) = std::malloc;
	inline void (*free)(void *ptr, size_t size) = detail::std_free_wrapper;

	inline MemoryArena *temporaryMemory = nullptr;

}

