#include "oak_util/memory.h"

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <oak_util/types.h>
#include <oak_util/ptr.h>
#include <oak_util/bit.h>

namespace oak {

	Result init_memory_arena(MemoryArena *arena, Allocator *allocator, u64 size) {
		assert(arena);
		assert(allocator);
		assert(size > sizeof(MemoryArenaHeader));

		auto block = allocator->allocate(size, 2048);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		auto header = static_cast<MemoryArenaHeader*>(block);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->next = nullptr;

		*arena = { block, size  };

		return Result::SUCCESS;
	}

	Result init_atomic_memory_arena(MemoryArena *arena, Allocator *allocator, u64 size) {
		assert(arena);
		assert(allocator);
		assert(size > sizeof(AtomicMemoryArenaHeader));

		auto block = allocator->allocate(size, 2048);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		auto header = static_cast<AtomicMemoryArenaHeader*>(block);
		header->allocationCount.store(0, std::memory_order_relaxed);
		header->requestedMemory.store(0, std::memory_order_relaxed);
		header->usedMemory.store(sizeof(AtomicMemoryArenaHeader), std::memory_order_relaxed);
		header->next = nullptr;

		*arena = { block, size };

		return Result::SUCCESS;
	}

	void destroy_memory_arena(MemoryArena *arena, Allocator *allocator) {
		allocator->deallocate(arena->block, arena->size);
		*arena = {};
	}

	void* allocate_from_arena(MemoryArena *arena, u64 size, u64 alignment) {
		assert(arena);
		assert(arena->block);
		assert(size > 0);
		assert(alignment > 0);

		auto header = static_cast<MemoryArenaHeader*>(arena->block);
		auto memoryToAllocate = size;

		auto offset = align_size(header->usedMemory, alignment);
		auto nUsedMemory = offset + memoryToAllocate;

		if (nUsedMemory < arena->size) {
			// If there was enough room for this allocation
			header->usedMemory = nUsedMemory;
			++header->allocationCount;
			header->requestedMemory += memoryToAllocate;

			return add_ptr(arena->block, offset);
		}

		return nullptr;
	}

	void* allocate_from_atomic_arena(MemoryArena *arena, u64 size, u64 alignment) {
		assert(arena);
		assert(arena->block);
		assert(size > 0);
		assert(alignment > 0);

		auto header = static_cast<AtomicMemoryArenaHeader*>(arena->block);
		auto memoryToAllocate = size;

		auto usedMemory = header->usedMemory.load(std::memory_order_relaxed);
		u64 offset, nUsedMemory;

		do {
			offset = align_size(usedMemory + memoryToAllocate, alignment);
			nUsedMemory = offset + memoryToAllocate;
		} while (nUsedMemory <= arena->size
				&& !header->usedMemory.compare_exchange_weak(
					usedMemory, nUsedMemory,
					std::memory_order_release, std::memory_order_relaxed));

		if (nUsedMemory <= arena->size) {
			// If there was enough room for this allocation
			header->allocationCount.fetch_add(1, std::memory_order_relaxed);
			header->requestedMemory.fetch_add(memoryToAllocate, std::memory_order_relaxed);

			return add_ptr(arena->block, offset);
		}

		return nullptr;
	}

	void copy_arena(MemoryArena *dst, MemoryArena *src) {
		assert(dst->size >= src->size);
		auto srcHeader = static_cast<MemoryArenaHeader*>(src->block);
		std::memcpy(dst->block, src->block, srcHeader->usedMemory);
	}

	void copy_atomic_arena(MemoryArena *dst, MemoryArena *src) {
		assert(dst->size >= src->size);
		auto srcHeader = static_cast<AtomicMemoryArenaHeader*>(src->block);
		std::memcpy(dst->block, src->block, srcHeader->usedMemory);
	}

	bool arena_contains(MemoryArena *arena, void *ptr) {
		auto start = reinterpret_cast<u64>(arena->block);
		auto end = reinterpret_cast<u64>(add_ptr(arena->block, arena->size));
		auto p = reinterpret_cast<u64>(ptr);

		return start < p && p <= end;
	}

	void clear_arena(MemoryArena *arena) {
		assert(arena);
		assert(arena->block);

		auto header = static_cast<MemoryArenaHeader*>(arena->block);

		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
	}

	void clear_atomic_arena(MemoryArena *arena) {
		assert(arena);
		assert(arena->block);

		auto header = static_cast<AtomicMemoryArenaHeader*>(arena->block);

		header->allocationCount.store(0, std::memory_order_relaxed);
		header->requestedMemory.store(0, std::memory_order_relaxed);
		header->usedMemory.store(sizeof(AtomicMemoryArenaHeader), std::memory_order_relaxed);
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
		++arenaHeader->allocationCount;
		return add_ptr(arena->block, arenaHeader->usedMemory);
	}

	void pop_stack(MemoryArena *arena, void *stackPtr) {
		auto arenaHeader = static_cast<MemoryArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(sub_ptr(stackPtr, sizeof(StackHeader)));
		auto ogUsedMemory = reinterpret_cast<u64>(stackPtr) - reinterpret_cast<u64>(arena->block) - sizeof(StackHeader);
		arenaHeader->usedMemory = ogUsedMemory;
		arenaHeader->allocationCount = stackHeader->allocationCount;
		arenaHeader->requestedMemory = stackHeader->requestedMemory;
	}

	Result init_memory_pool(MemoryArena *arena, Allocator *allocator, u64 size, u64 alignment) {
		auto const poolSize = ensure_pow2(size);
		auto const totalSize = poolSize + align_size(sizeof(PoolHeader), alignment);

		auto block = allocator->allocate(totalSize, alignment);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		*arena = {
			block,
			totalSize
		};

		auto poolNode = static_cast<PoolNode*>(add_ptr(block, align_size(sizeof(PoolHeader), alignment)));
		*poolNode = {
			nullptr,
			poolSize
		};

		auto poolHeader = static_cast<PoolHeader*>(block);
		*poolHeader = {
			poolNode,
			poolSize,
			alignment
		};

		return Result::SUCCESS;
	}

	void* allocate_from_pool(MemoryArena *arena, u64 size, u64) {
		auto poolHeader = static_cast<PoolHeader*>(arena->block);

		auto targetNodeSize = ensure_pow2(align_size(size, poolHeader->alignment));
		if (targetNodeSize < sizeof(PoolNode)) {
			targetNodeSize = sizeof(PoolNode);
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

	void coalesce_memory_pool(void const *block, PoolNode **nodePtr) {
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
				// If nodes are neightbours and the two nodes combined have the correct alignment
				if (add_ptr(firstNode, firstNode->size) == secondNode &&
						firstNode->size == secondNode->size &&
						align_size(rfn, firstNode->size + secondNode->size) == rfn) {
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

	void free_from_pool(MemoryArena *arena, void *ptr, u64 size) {
		assert(arena->block < ptr && ptr < add_ptr(arena->block, arena->size));

		auto poolHeader = static_cast<PoolHeader*>(arena->block);
		auto nodeSize = ensure_pow2(align_size(size, poolHeader->alignment));
		if (nodeSize < sizeof(PoolNode)) {
			nodeSize = sizeof(PoolNode);
		}

		// Insert node into freelist sorted by ptr
		auto poolNodePtr = &poolHeader->freeList;
		while ((*poolNodePtr) && (*poolNodePtr) < ptr) {
			poolNodePtr = &(*poolNodePtr)->next;
		}
		auto node = static_cast<PoolNode*>(ptr);
		node->next = (*poolNodePtr);
		node->size = nodeSize;
		(*poolNodePtr) = node;

		coalesce_memory_pool(add_ptr(arena->block, sizeof(PoolHeader)), poolNodePtr);
	}

namespace detail {


	void* std_aligned_alloc_wrapper(MemoryArena*, u64 size, u64 align) {
		//return std::aligned_alloc(align, align_size(size, align));
		return std::malloc(align_size(size, align));
	}

	void std_free_wrapper(MemoryArena*, void *ptr, u64) {
		std::free(ptr);
	}

}

}

