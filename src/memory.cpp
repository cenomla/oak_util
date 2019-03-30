#include "oak_util/memory.h"

#include <cassert>
#include <cstring>

#include <oak_util/ptr.h>
#include <oak_util/bit.h>

namespace oak {

	Result init_memory_arena(MemoryArena *arena, size_t size) {
		assert(arena);

		auto block = alloc(size);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		return init_memory_arena(arena, block, size);
	}

	Result init_memory_arena(MemoryArena *arena, void *block, size_t size) {
		assert(arena);
		assert(block);
		assert(size > sizeof(MemoryArenaHeader));

		auto header = static_cast<MemoryArenaHeader*>(block);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->next = nullptr;

		*arena = { block, size };

		return Result::SUCCESS;
	}

	void* allocate_from_arena(MemoryArena *arena, size_t size, int64_t count, size_t alignment) {
		assert(arena);
		assert(arena->block);
		assert(size > 0);

		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		auto memoryToAllocate = size * count;

		if (memoryToAllocate && header->usedMemory + memoryToAllocate < arena->size) {
			auto result = add_ptr(arena->block, header->usedMemory);
			auto offset = align_offset(result, alignment);

			++header->allocationCount;
			header->requestedMemory += memoryToAllocate;
			header->usedMemory += offset + memoryToAllocate;

			return add_ptr(result, offset);
		}

		return nullptr;
	}

	void copy_arena(MemoryArena *dst, MemoryArena *src) {
		assert(dst->size >= src->size);
		auto srcHeader = static_cast<MemoryArenaHeader*>(src->block);
		std::memcpy(dst->block, src->block, srcHeader->usedMemory);
	}

	void clear_arena(MemoryArena *arena) {
		assert(arena);
		assert(arena->block);

		auto header = static_cast<MemoryArenaHeader*>(arena->block);

		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
	}

	bool arena_contains(MemoryArena *arena, void *ptr) {
		auto start = reinterpret_cast<uint64_t>(arena->block);
		auto end = reinterpret_cast<uint64_t>(add_ptr(arena->block, arena->size));
		auto p = reinterpret_cast<uint64_t>(ptr);
		return start < p && p <= end;
	}

	void* push_stack(MemoryArena *arena) {
		auto arenaHeader = static_cast<MemoryArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(add_ptr(arena->block, arenaHeader->usedMemory));
		// Save header state
		stackHeader->allocationCount = arenaHeader->allocationCount;
		stackHeader->requestedMemory = arenaHeader->requestedMemory;
		// Track this allocation
		arenaHeader->usedMemory += ssizeof(StackHeader);
		arenaHeader->requestedMemory += ssizeof(StackHeader);
		arenaHeader->allocationCount ++;
		return add_ptr(arena->block, arenaHeader->usedMemory);
	}

	void pop_stack(MemoryArena *arena, void *stackPtr) {
		auto arenaHeader = static_cast<MemoryArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(sub_ptr(stackPtr, sizeof(StackHeader)));
		auto ogUsedMemory = reinterpret_cast<intptr_t>(stackPtr) - reinterpret_cast<intptr_t>(arena->block) - ssizeof(StackHeader);
		arenaHeader->usedMemory = ogUsedMemory;
		arenaHeader->allocationCount = stackHeader->allocationCount;
		arenaHeader->requestedMemory = stackHeader->requestedMemory;
	}

	MemoryArena create_pool(MemoryArena *arena, size_t size) {
		auto const poolSize = ensure_pow2(size);
		auto const totalSize = poolSize + sizeof(PoolHeader);
		auto block = allocate_from_arena(arena, totalSize, 1, sizeof(void*));
		MemoryArena poolArena;
		poolArena.block = block;
		poolArena.size = totalSize;
		auto poolNode = static_cast<PoolNode*>(add_ptr(poolArena.block, sizeof(PoolHeader)));
		poolNode->next = nullptr;
		poolNode->size = poolSize;
		auto poolHeader = static_cast<PoolHeader*>(block);
		poolHeader->freeList = poolNode;
		poolHeader->poolSize = poolSize;
		return poolArena;
	}

	void* allocate_from_pool(MemoryArena *arena, size_t size, int64_t count) {
		auto poolHeader = static_cast<PoolHeader*>(arena->block);
		auto totalSize = size * count;
		auto targetNodeSize = ensure_pow2(totalSize);
		if (targetNodeSize < 16) {
			targetNodeSize = 16;
		}
		// Get the first node that is bigger than or equal to the target node size
		auto poolNodePtr = &poolHeader->freeList;
		while ((*poolNodePtr) && (*poolNodePtr)->size < targetNodeSize) {
			poolNodePtr = &(*poolNodePtr)->next;
		}
		if (!(*poolNodePtr)) {
			// There are no nodes big enough to fit the memory in it so return nullptr
			return nullptr;
		}
		// We now have a node big enough to fit the allocation
		// If the node is bigger than the target size split it in half until it matches the target size
		while ((*poolNodePtr)->size > targetNodeSize) {
			// Split the node
			auto halfSize = (*poolNodePtr)->size / 2;
			auto firstNode = *poolNodePtr;
			auto secondNode = static_cast<PoolNode*>(add_ptr(firstNode, halfSize));
			firstNode->size = halfSize;
			secondNode->size = halfSize;
			// Add the second node to the freeList
			auto next = firstNode->next;
			firstNode->next = secondNode;
			secondNode->next = next;
		}
		assert((*poolNodePtr)->size == targetNodeSize);
		// Remove this node from the freelist and return its pointer
		auto node = static_cast<void*>(*poolNodePtr);
		*poolNodePtr = (*poolNodePtr)->next;
		return node;
	}

namespace {

	void coalesce_memory_pool(const void *block, PoolNode **nodePtr) {
		if (!*nodePtr) { return; }
		// nodePtr is a pointer to the first node (the address of the next variable from the previous node)
		auto nodeSize = (*nodePtr)->size;
		PoolNode **searchNode;
		do {
			// Use search node to find the last free block with the node size
			searchNode = nodePtr;
			while (true) {
				if (!(*searchNode)->next || //next node doesnt exist
						(*searchNode)->next->size != nodeSize || //next node has different size
						add_ptr(*searchNode, (*searchNode)->size) != (*searchNode)->next) { //next node is not adjacent
					break;
				}
				searchNode = &(*searchNode)->next;
			}
			// searchNode now points to the the next field of the node previous
			// Since the next node is the first field of the struct a pointer to it is a pointer to the struct
			auto firstNode = reinterpret_cast<PoolNode*>(searchNode);
			auto secondNode = firstNode->next;
			// Check if we should collapse these nodes
			while (secondNode) {
				auto rfn = ptr_diff(block, firstNode);
				if (add_ptr(firstNode, firstNode->size) == secondNode && //nodes are neighbours
						firstNode->size == secondNode->size &&
						align_size(rfn, firstNode->size + secondNode->size) == rfn) { //alignment is satisfied
					firstNode->next = secondNode->next;
					firstNode->size += secondNode->size;
					secondNode = secondNode->next;
				} else {
					break;
				}
			}
		} while (searchNode != nodePtr);
	}

}

	void free_from_pool(MemoryArena *arena, const void *ptr, size_t size, int64_t count) {
		assert(arena->block < ptr && ptr < add_ptr(arena->block, arena->size));
		auto poolHeader = static_cast<PoolHeader*>(arena->block);
		auto totalSize = size * count;
		auto nodeSize = ensure_pow2(totalSize);
		if (nodeSize < 16) {
			nodeSize = 16;
		}
		// Insert node into freelist sorted by ptr
		auto poolNodePtr = &poolHeader->freeList;
		while ((*poolNodePtr) && (*poolNodePtr) < ptr) {
			poolNodePtr = &(*poolNodePtr)->next;
		}
		auto node = static_cast<PoolNode*>(const_cast<void *>(ptr));
		node->next = (*poolNodePtr);
		node->size = nodeSize;
		(*poolNodePtr) = node;

		coalesce_memory_pool(add_ptr(arena->block, sizeof(PoolHeader)), poolNodePtr);
	}

}

