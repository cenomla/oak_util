#pragma once

#include "ptr.h"

namespace oak {

	struct MemoryArenaHeader {
		size_t alignment;
		int64_t allocationCount;
		int64_t allocatedMemory;
		int64_t usedMemory;
		void *next;
	};

	struct MemoryArena {
		void *block = nullptr;
		size_t size = 0;
	};

	MemoryArena create_memory_arena(size_t size, size_t alignment);
	void destroy_memory_arena(MemoryArena *arena);
	void *allocate_from_arena(MemoryArena *arena, size_t size, size_t count);
	void clear_arena(MemoryArena *arena);

	template<typename T, typename... TArgs>
	T* make(MemoryArena *arena, size_t count, TArgs&&... parameters) {
		auto mem = allocate_from_arena(arena, sizeof(T), count);
		new (mem) T{ parameters... };
		return static_cast<T*>(mem);
	}

}
