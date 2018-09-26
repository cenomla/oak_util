#pragma once

#include "ptr.h"

namespace oak {

	struct MemoryArenaHeader {
		int64_t allocationCount;
		int64_t allocatedMemory;
		int64_t usedMemory;
		void *next;
	};

	struct StackHeader {
		int64_t allocationCount;
		int64_t allocatedMemory;
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

	MemoryArena create_memory_arena(size_t size);
	void destroy_memory_arena(MemoryArena *arena);
	MemoryArena create_memory_arena(void *block, size_t size);

	void *allocate_from_arena(MemoryArena *arena, size_t size, int64_t count, size_t alignment);
	void clear_arena(MemoryArena *arena);

	void* push_stack(MemoryArena *arena);
	void pop_stack(MemoryArena *arena, void *stackPtr);

	MemoryArena create_pool(MemoryArena *arena, size_t size);
	void* allocate_from_pool(MemoryArena *arena, size_t size, int64_t count);
	void free_from_pool(MemoryArena *arena, const void *ptr, size_t size, int64_t count);

	struct ArenaStack {
		MemoryArena *arena = nullptr;
		void *stackPtr = nullptr;

		ArenaStack(MemoryArena *arena_) : arena{ arena_ }, stackPtr{ push_stack(arena) } {}

		~ArenaStack() {
			pop_stack(arena, stackPtr);
		}
	};

	template<typename T>
	T* allocate_structs(MemoryArena *arena, int64_t count) {
		auto structsPtr = static_cast<T*>(allocate_from_arena(arena, sizeof(T), count, alignof(T)));
		return structsPtr;
	}

	template<typename T, typename... TArgs>
	T* make_structs(MemoryArena *arena, int64_t count, TArgs&&... parameters) {
		auto structsPtr = static_cast<T*>(allocate_from_arena(arena, sizeof(T), count, alignof(T)));
		for (int64_t i = 0; i < count; i++) {
			structsPtr[i] = { parameters... };
		}
		return structsPtr;
	}

	inline MemoryArena *temporaryMemory = nullptr;

}

