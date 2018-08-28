#include "memory.h"

#include <cassert>
#include <cstdlib>

#include "type_info.h"

namespace oak {

	MemoryArena create_memory_arena(size_t size) {
		auto totalSize = sizeof(MemoryArenaHeader) + size;
		auto block = calloc(totalSize, 1);
		if (!block) {
			return {};
		}
		return create_memory_arena(block, totalSize);
	}

	MemoryArena create_memory_arena(void *block, size_t size) {
		assert(block);
		assert(size > sizeof(MemoryArenaHeader));
		auto header = static_cast<MemoryArenaHeader*>(block);
		header->allocationCount = 0;
		header->allocatedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->next = nullptr;
		MemoryArena arena{ block, size };
		return arena;
	}

	void destroy_memory_arena(MemoryArena *arena) {
		free(arena->block);
		arena->block = nullptr;
		arena->size = 0;
	}

	void *allocate_from_arena(MemoryArena *arena, size_t size, int64_t count, size_t alignment) {
		assert(arena);
		assert(arena->block);
		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		auto alignedSize = align_size(size, alignment);
		auto memoryToAllocate = alignedSize * count;
		if (header->usedMemory + memoryToAllocate < arena->size) {
			auto result = add_ptr(arena->block, header->usedMemory);
			auto offset = align_offset(result, alignment);
			header->allocationCount++;
			header->allocatedMemory += size * count;
			header->usedMemory += offset + memoryToAllocate;
			return add_ptr(result, offset);
		}
		//we have to more memory left in the arena so return nullptr for now
		//TODO: should we handle arena resizing/expanding?
		return nullptr;
	}

	void clear_arena(MemoryArena *arena) {
		assert(arena);
		assert(arena->block);
		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		header->allocationCount = 0;
		header->allocatedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
	}

	void* push_stack(MemoryArena *arena) {
		auto arenaHeader = static_cast<MemoryArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(add_ptr(arena->block, arenaHeader->usedMemory));
		//save header state
		stackHeader->allocationCount = arenaHeader->allocationCount;
		stackHeader->allocatedMemory = arenaHeader->allocatedMemory;
		//track this allocation
		arenaHeader->usedMemory += ssizeof(StackHeader);
		arenaHeader->allocatedMemory += ssizeof(StackHeader);
		arenaHeader->allocationCount ++;
		return add_ptr(arena->block, arenaHeader->usedMemory);
	}

	void pop_stack(MemoryArena *arena, void *stackPtr) {
		auto arenaHeader = static_cast<MemoryArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(sub_ptr(stackPtr, sizeof(StackHeader)));
		auto ogUsedMemory = reinterpret_cast<intptr_t>(stackPtr) - reinterpret_cast<intptr_t>(arena->block) - ssizeof(StackHeader);
		arenaHeader->usedMemory = ogUsedMemory;
		arenaHeader->allocationCount = stackHeader->allocationCount;
		arenaHeader->allocatedMemory = stackHeader->allocatedMemory;
	}

}

