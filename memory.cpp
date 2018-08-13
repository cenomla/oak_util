#include "memory.h"

#include <cassert>
#include <cstdlib>

namespace oak {

	MemoryArena create_memory_arena(size_t size, size_t alignment) {
		auto headerSize = align_size(sizeof(MemoryArenaHeader), alignment);
		auto alignedSize = align_size(size, alignment);
		auto block = aligned_alloc(alignment, headerSize + alignedSize);
		if (!block) {
			return {};
		}
		auto header = static_cast<MemoryArenaHeader*>(block);
		header->alignment = alignment;
		header->allocationCount = 0;
		header->allocatedMemory = 0;
		header->usedMemory = 0;
		header->next = nullptr;
		MemoryArena arena{ block, alignedSize };
		return arena;
	}

	void destroy_memory_arena(MemoryArena *arena) {
		free(arena->block);
		arena->block = nullptr;
		arena->size = 0;
	}

	void *allocate_from_arena(MemoryArena *arena, size_t size, size_t count) {
		assert(arena && arena->block);
		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		auto alignedSize = align_size(size, header->alignment);
		auto memoryToAllocate = alignedSize * count;
		if (header->usedMemory + memoryToAllocate < arena->size) {
			auto blockOffset = align_size(sizeof(MemoryArenaHeader), header->alignment) + header->usedMemory;
			auto result = add_ptr(arena->block, blockOffset);
			header->allocationCount++;
			header->allocatedMemory += size * count;
			header->usedMemory += memoryToAllocate;
			return result;
		}
		//we have to more memory left in the arena so return nullptr for now
		//TODO: should we handle arena resizing/expanding?
		return nullptr;
	}

	void clear_arena(MemoryArena *arena) {
		assert(arena && arena->block);
		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		header->allocationCount = 0;
		header->allocatedMemory = 0;
		header->usedMemory = 0;
	}

}

