#pragma once

#include <new>

#include "types.h"
#include "ptr.h"

namespace oak {

	struct MemoryArena {
		void *block = nullptr;
		u64 size = 0;
	};

	struct LinearArenaHeader {
		u64 allocationCount;
		u64 requestedMemory;
		u64 usedMemory;
		void *next;
	};

	struct MTHeapBlockHeader {
		MTHeapBlockHeader *next = nullptr;
		u64 allocationCount = 0;
		u64 requestedMemory = 0;
		u64 usedMemory = 0;
	};

	struct MTHeapArenaHeader {
		Allocator *allocator = nullptr;
		u64 blockSize = 0;

		u64 totalAllocationCount = 0;
		u64 totalRequestedMemory = 0;
		u64 totalUsedMemory = 0;
		u64 usedBlocks = 0;
		u64 totalBlocks = 0;

		alignas(64) i32 _lock = 0;
		MTHeapBlockHeader *freeHeapList = nullptr;
	};

	struct RingArenaHeader {
		u32 offset;
		u64 allocationCount;
		u64 requestedMemory;
		u32 usedMemory;
		void *next;
	};

	struct StackHeader {
		i64 allocationCount;
		i64 requestedMemory;
	};

	struct PoolNode {
		PoolNode *next;
		u64 size;
	};

	struct PoolHeader {
		PoolNode *freeList;
		u64 poolSize;
		u64 alignment;
	};

	struct Allocator {
		MemoryArena *arena = nullptr;
		void* (*allocFn)(MemoryArena *self, u64 size, u64 alignment) = nullptr;
		void (*freeFn)(MemoryArena *self, void *ptr, u64 size) = nullptr;
		void (*clearFn)(MemoryArena *self) = nullptr;

		void* allocate(u64 size, u64 alignment) {
			return (*allocFn)(arena, size, alignment);
		}

		void deallocate(void *ptr, u64 size) {
			(*freeFn)(arena, ptr, size);
		}

		void clear() {
			(*clearFn)(arena);
		}
	};

	OAK_UTIL_API Result init_linear_arena(MemoryArena *arena, Allocator *allocator, u64 size);
	OAK_UTIL_API Result init_linear_arena(MemoryArena *arena, void *ptr, u64 size);
	OAK_UTIL_API Result init_atomic_linear_arena(MemoryArena *arena, Allocator *allocator, u64 size);
	OAK_UTIL_API void* allocate_from_linear_arena(MemoryArena *arena, u64 size, u64 alignment);
	OAK_UTIL_API void* allocate_from_atomic_linear_arena(MemoryArena *arena, u64 size, u64 alignment);
	OAK_UTIL_API void free_from_linear_arena(MemoryArena *arena, void *ptr, u64 size);
	OAK_UTIL_API void free_from_atomic_linear_arena(MemoryArena *arena, void *ptr, u64 size);
	OAK_UTIL_API Result copy_linear_arena(MemoryArena *dst, MemoryArena *src);
	OAK_UTIL_API Result copy_atomic_linear_arena(MemoryArena *dst, MemoryArena *src);
	OAK_UTIL_API void clear_linear_arena(MemoryArena *arena);
	OAK_UTIL_API void clear_atomic_linear_arena(MemoryArena *arena);

	OAK_UTIL_API i32 init_heap_linear_arena(MemoryArena *arena, Allocator *allocator, u64 blockSize);
	OAK_UTIL_API void* allocate_from_heap_linear_arena(MemoryArena *arena, u64 size, u64 alignment);
	OAK_UTIL_API void free_from_heap_linear_arena(MemoryArena *arena, void *ptr, u64 size);
	OAK_UTIL_API void clear_heap_linear_arena(MemoryArena *arena);

	OAK_UTIL_API Result init_ring_arena(MemoryArena *arena, Allocator *allocator, u64 size);
	OAK_UTIL_API void* allocate_from_ring_arena(MemoryArena *arena, u64 size, u64 alignment);
	OAK_UTIL_API void deallocate_from_ring_arena(MemoryArena *arena, void *ptr, u64 size);
	OAK_UTIL_API void clear_ring_arena(MemoryArena *arena);

	OAK_UTIL_API void destroy_arena(MemoryArena *arena, Allocator *allocator);
	OAK_UTIL_API bool arena_contains(MemoryArena *arena, void *ptr);

	OAK_UTIL_API void* push_stack(MemoryArena *arena);
	OAK_UTIL_API void pop_stack(MemoryArena *arena, void *stackPtr);

	OAK_UTIL_API Result init_memory_pool(MemoryArena *arena, Allocator *allocator, u64 size, u64 alignment);

	OAK_UTIL_API void* allocate_from_pool(MemoryArena *arena, u64 size, u64 alignment);
	OAK_UTIL_API void free_from_pool(MemoryArena *arena, void *ptr, u64 size);

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

	template<typename... types>
	void* allocate_soa(Allocator *allocator, i64 count) {
		return allocator->allocate(soa_offset<sizeof...(types), types...>(count), max_align<types...>());
	}

	template<typename T>
	void deallocate(Allocator *allocator, T *ptr, i64 count) {
		allocator->deallocate(static_cast<void*>(ptr), sizeof(T) * count);
	}

	template<typename T>
	void deallocate(Allocator *allocator, T const *ptr, i64 count) {
		allocator->deallocate(const_cast<void*>(static_cast<void const*>(ptr)), sizeof(T) * count);
	}

	template<typename... types>
	void deallocate_soa(Allocator *allocator, void *ptr, i64 count) {
		allocator->deallocate(ptr, soa_offset<sizeof...(types), types...>(count));
	}

	template<typename T, typename... TArgs>
	T* make(Allocator *allocator, i64 count, TArgs&&... args) {
		auto result = allocate<T>(allocator, count);
		if (result) {
			for (i64 i = 0; i < count; ++i) {
				new (result + i) T{ static_cast<TArgs&&>(args)... };
			}
		}
		return result;
	}

	template<typename T>
	void destroy(Allocator *allocator, T *ptr, i64 count) {
		for (i64 i = 0; i < count; ++i) {
			ptr[i].~T();
		}
		deallocate<T>(allocator, ptr, count);
	}

	OAK_UTIL_API void* std_aligned_alloc_wrapper(MemoryArena*, u64 size, u64 align);
	OAK_UTIL_API void std_free_wrapper(MemoryArena*, void *ptr, u64);
	OAK_UTIL_API void std_clear_wrapper(MemoryArena*);

	OAK_UTIL_API extern Allocator* globalAllocator;
	OAK_UTIL_API extern Allocator* temporaryAllocator;

	#define TMP_ALLOC(size)\
		u8 tmpMemory[sizeof(LinearArenaHeader) + size];\
		MemoryArena tmpArena;\
		Allocator tmpAlloc{ &tmpArena, allocate_from_linear_arena, free_from_linear_arena };\
		init_linear_arena(&tmpArena, tmpMemory, sizeof(tmpMemory))

}

