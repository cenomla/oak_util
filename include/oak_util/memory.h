#pragma once

#include <cstdlib>
#include <cstring>
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

	Result init_memory_arena(MemoryArena *arena, size_t size);
	Result init_memory_arena(MemoryArena *arena, void *block, size_t size);

	void* allocate_from_arena(MemoryArena *arena, size_t size, size_t alignment);
	void clear_arena(MemoryArena *arena);

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
	T* allocate(Allocator *allocator, i64 count) {
		return static_cast<T*>(allocator->allocate(sizeof(T) * count, alignof(T)));
	}

	template<typename T>
	void deallocate(Allocator *allocator, T *ptr, i64 count) {
		allocator->deallocate(static_cast<void*>(ptr), count);
	}

	template<typename T, typename... TArgs>
	T* make(Allocator *allocator, i64 count, TArgs&&... parameters) {
		auto result = allocate<T>(allocator, count);
		if (result) {
			for (i64 i = 0; i < count; ++i) {
				new (result + i) T{ static_cast<TArgs&&>(parameters)... };
			}
		}
		return result;
	}

	namespace detail {

		inline void* arena_alloc_wrapper(void* arena, u64 size, u64 align) {
			return allocate_from_arena(static_cast<MemoryArena*>(arena), size, align);
		}

		inline void* std_aligned_alloc_wrapper(void*, u64 size, u64 align) {
			return std::aligned_alloc(align, size);
		}

		inline void std_free_wrapper(void *, void *ptr, u64) {
			std::free(ptr);
		}

	}

	inline Allocator globalAllocator{ nullptr, detail::std_aligned_alloc_wrapper, detail::std_free_wrapper };
	inline Allocator temporaryMemory;

}

