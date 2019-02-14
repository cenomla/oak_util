#include "oak_util/memory.h"

#include <cassert>

#include <oak_util/allocator.h>
#include <oak_util/type_info.h>
#include <oak_util/bit.h>

namespace oak {

	MemoryArena create_memory_arena(size_t size) {
		auto totalSize = sizeof(MemoryArenaHeader) + size;
		auto block = malloc(totalSize);
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
		auto memoryToAllocate = size * count;
		if (header->usedMemory + memoryToAllocate < arena->size) {
			auto result = add_ptr(arena->block, header->usedMemory);
			auto offset = align_offset(result, alignment);
			header->allocationCount++;
			header->allocatedMemory += size * count;
			header->usedMemory += offset + memoryToAllocate;
			return add_ptr(result, offset);
		}
		assert("out of memory" && false);
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

	MemoryArena create_pool(MemoryArena *arena, size_t size) {
		const auto poolSize = ensure_pow2(size);
		const auto totalSize = poolSize + sizeof(PoolHeader);
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
		//get the first node that is bigger than or equal to the target node size
		auto poolNodePtr = &poolHeader->freeList;
		while ((*poolNodePtr) && (*poolNodePtr)->size < targetNodeSize) {
			poolNodePtr = &(*poolNodePtr)->next;
		}
		if (!(*poolNodePtr)) {
			//there are no nodes big enough to fit the memory in it so return nullptr
			return nullptr;
		}
		//we now have a node big enough to fit the allocation
		//if the node is bigger than the target size split it in half until it matches the target size
		while ((*poolNodePtr)->size > targetNodeSize) {
			//split the node
			auto halfSize = (*poolNodePtr)->size / 2;
			auto firstNode = *poolNodePtr;
			auto secondNode = static_cast<PoolNode*>(add_ptr(firstNode, halfSize));
			firstNode->size = halfSize;
			secondNode->size = halfSize;
			//add the second node to the freeList
			auto next = firstNode->next;
			firstNode->next = secondNode;
			secondNode->next = next;
		}
		assert((*poolNodePtr)->size == targetNodeSize);
		//remove this node from the freelist and return its pointer
		auto node = static_cast<void*>(*poolNodePtr);
		*poolNodePtr = (*poolNodePtr)->next;
		return node;
	}

namespace {

	void coalesce_memory_pool(const void *block, PoolNode **nodePtr) {
		if (!*nodePtr) { return; }
		//nodePtr is a pointer to the first node (the address of the next variable from the previous node)
		auto nodeSize = (*nodePtr)->size;
		PoolNode **searchNode;
		do {
			//use search node to find the last free block with the node size
			searchNode = nodePtr;
			while (true) {
				if (!(*searchNode)->next || //next node doesnt exist
						(*searchNode)->next->size != nodeSize || //next node has different size
						add_ptr(*searchNode, (*searchNode)->size) != (*searchNode)->next) { //next node is not adjacent
					break;
				}
				searchNode = &(*searchNode)->next;
			}
			//searchNode now points to the the next field of the node previous
			//since the next node is the first field of the struct a pointer to it is a pointer to the struct
			auto firstNode = reinterpret_cast<PoolNode*>(searchNode);
			auto secondNode = firstNode->next;
			//check if we should collapse these nodes
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
		//insert node into freelist sorted by ptr
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

