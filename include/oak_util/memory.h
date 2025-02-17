#pragma once

#include <new>

#include "types.h"
#include "ptr.h"

namespace oak {

	struct MemoryArena;

	struct MemoryArenaHeader {
		enum FlagBits : u32 {
			CHAINED_BIT = 0x1,
			SUB_ALLOCATED_BIT = 0x2,
		};

		usize capacity = 0;
		usize usedMemory = 0;
		usize commitSize = 0;
		usize pageSize = 0;
		void *next = nullptr;
		void *last = nullptr;
		usize alignSize = 0;
		u32 flags = 0;

		// Debug info
		i64 allocationCount = 0;
		usize requestedMemory = 0;

		// Thread synchronization
		alignas(64) i32 _lock = 0;
		MemoryArena *_nextArena = nullptr;
		u64 _threadId = 0;
	};

	struct MemoryPoolHeader {
		usize objectSize = 0;
		void *freeList = nullptr;
	};

	struct MemoryHeapHeader {
		usize minPoolObjectSize = 0;
		usize maxPoolObjectSize = 0;
		usize heapSmallPageSize = 0;
		usize heapLargePageSize = 0;

		void *poolFreeLists[16] = {};
	};

	struct MTMemoryArenaHeader {
		usize threadArenaSize = 0;

		u64 totalAllocationCount = 0;
		u64 totalRequestedMemory = 0;
		u64 totalUsedMemory = 0;

		MemoryArena *first = nullptr;
		MemoryArena *last = nullptr;

		alignas(64) i32 _lock = 0;
	};

	struct Allocator {
		MemoryArena *arena = nullptr;
		void* (*allocFn)(MemoryArena *self, u64 size, u64 alignment) = nullptr;
		void (*freeFn)(MemoryArena *self, void *ptr, u64 size) = nullptr;
		void* (*reallocFn)(MemoryArena *self, void *ptr, u64 size, u64 nSize, u64 alignment) = nullptr;
		void (*clearFn)(MemoryArena *self) = nullptr;

		inline void* allocate(u64 size, u64 alignment) {
			return (*allocFn)(arena, size, alignment);
		}

		inline void deallocate(void *ptr, u64 size) {
			(*freeFn)(arena, ptr, size);
		}

		inline void* realloc(void *ptr, u64 size, u64 nSize, u64 alignment) {
			return (*reallocFn)(arena, ptr, size, nSize, alignment);
		}

		inline void clear() {
			(*clearFn)(arena);
		}
	};

	OAK_UTIL_API void* virtual_alloc(usize size);
	OAK_UTIL_API bool virtual_try_grow(void *addr, usize size, usize nSize);
	OAK_UTIL_API void virtual_free(void *addr, usize size);
	OAK_UTIL_API i32 commit_region(void *addr, usize size);
	OAK_UTIL_API i32 decommit_region(void *addr, usize size);

	OAK_UTIL_API i32 memory_arena_init(MemoryArena **arena, usize size);
	OAK_UTIL_API i32 memory_arena_init(MemoryArena **arena, void *addr, usize size);
	OAK_UTIL_API void memory_arena_align_size(MemoryArena *arena, usize alignSize);
	OAK_UTIL_API void memory_arena_destroy(MemoryArena *arena);
	OAK_UTIL_API void* memory_arena_alloc(MemoryArena *arena, usize size, usize alignment);
	OAK_UTIL_API void memory_arena_free(MemoryArena *arena, void *addr, usize size);
	OAK_UTIL_API void* memory_arena_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment);
	OAK_UTIL_API void memory_arena_clear(MemoryArena *arena);

	OAK_UTIL_API i32 memory_pool_init(MemoryArena **arena, usize size, usize objectSize);
	OAK_UTIL_API void memory_pool_destroy(MemoryArena *arena);
	OAK_UTIL_API void* memory_pool_alloc(MemoryArena *arena, usize size, usize alignment);
	OAK_UTIL_API void memory_pool_free(MemoryArena *arena, void *addr, usize size);
	OAK_UTIL_API void* memory_pool_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment);
	OAK_UTIL_API void memory_pool_clear(MemoryArena *arena);
	OAK_UTIL_API usize memory_pool_get_object_size(MemoryArena *arena);

	OAK_UTIL_API i32 memory_heap_init(MemoryArena **arena, usize size);
	OAK_UTIL_API void* memory_heap_alloc(MemoryArena *arena, usize size, usize alignment);
	OAK_UTIL_API void memory_heap_free(MemoryArena *arena, void *addr, usize size);
	OAK_UTIL_API void* memory_heap_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment);
	OAK_UTIL_API void memory_heap_clear(MemoryArena *arena);

	OAK_UTIL_API i32 mt_memory_arena_init(MemoryArena **arena, usize size);
	OAK_UTIL_API void mt_memory_arena_destroy(MemoryArena *arena);
	OAK_UTIL_API void* mt_memory_arena_alloc(MemoryArena *arena, usize size, usize alignment);
	OAK_UTIL_API void mt_memory_arena_free(MemoryArena *arena, void *addr, usize size);
	OAK_UTIL_API void* mt_memory_arena_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment);
	OAK_UTIL_API void mt_memory_arena_clear(MemoryArena *arena);

	OAK_UTIL_API i32 sys_alloc_init(MemoryArena **arena);
	OAK_UTIL_API void sys_alloc_destroy(MemoryArena *arena);
	OAK_UTIL_API void* sys_alloc(MemoryArena *arena, usize size, usize alignment);
	OAK_UTIL_API void sys_free(MemoryArena *arena, void *addr, usize size);
	OAK_UTIL_API void* sys_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment);
	OAK_UTIL_API void sys_clear(MemoryArena *arena);

	OAK_UTIL_API Allocator make_arena_allocator(usize size);
	OAK_UTIL_API Allocator make_arena_allocator(void *addr, usize size);
	OAK_UTIL_API Allocator make_pool_allocator(usize size, usize objectSize);
	OAK_UTIL_API Allocator make_heap_allocator(usize size);
	OAK_UTIL_API Allocator make_mt_arena_allocator(usize size);
	OAK_UTIL_API Allocator make_sys_allocator();

	// For use with APIs that don't support size based allocator interfaces
	OAK_UTIL_API void* global_allocator_malloc(usize size);
	OAK_UTIL_API void* global_allocator_realloc(void *ptr, usize size);
	OAK_UTIL_API void global_allocator_free(void *ptr);

	OAK_UTIL_API void* temporary_allocator_malloc(usize size);
	OAK_UTIL_API void* temporary_allocator_realloc(void *ptr, usize size);
	OAK_UTIL_API void temporary_allocator_free(void *ptr);

	template<typename T>
	void mem_copy(T *dst, T const *src, i64 count) {
		memcpy(dst, src, sizeof(T)*count);
	}

	template<typename T>
	void mem_move(T *dst, T const *src, i64 count) {
		memmove(dst, src, sizeof(T)*count);
	}

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

	template<typename T>
	T* reallocate(Allocator *allocator, T *ptr, i64 count, i64 nCount) {
		return static_cast<T*>(allocator->realloc(ptr, sizeof(T) * count, sizeof(T) * nCount, alignof(T)));
	}

	template<typename... types>
	void* reallocate_soa(Allocator *allocator, void *ptr, i64 count, i64 nCount) {
		return allocator->realloc(
				ptr,
				soa_offset<sizeof...(types), types...>(count),
				soa_offset<sizeof...(types), types...>(nCount),
				max_align<types...>());
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

	OAK_UTIL_API inline Allocator* globalAllocator = nullptr;
	OAK_UTIL_API inline Allocator* temporaryAllocator = nullptr;

	#define TMP_ALLOC(size)\
		alignas(64) u8 _tmpMemory[sizeof(MemoryArenaHeader) + size];\
		Allocator tmpAlloc = make_arena_allocator(_tmpMemory, sizeof(_tmpMemory))

}

