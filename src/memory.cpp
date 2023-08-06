#define OAK_UTIL_EXPORT_SYMBOLS

#include <oak_util/memory.h>
#include <oak_util/atomic.h>

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <oak_util/types.h>
#include <oak_util/ptr.h>
#include <oak_util/bit.h>

namespace oak {

	Result init_linear_arena(MemoryArena *arena, Allocator *allocator, u64 size) {
		if (size < sizeof(LinearArenaHeader))
			return Result::INVALID_ARGS;

		size = align(size, 2048);

		auto block = allocator->allocate(size, 2048);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		auto header = static_cast<LinearArenaHeader*>(block);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(LinearArenaHeader);
		header->next = nullptr;

		*arena = { block, size  };

		return Result::SUCCESS;
	}

	Result init_linear_arena(MemoryArena *arena, void *ptr, u64 size) {
		if (size < sizeof(LinearArenaHeader)) {
			return Result::INVALID_ARGS;
		}

		auto header = static_cast<LinearArenaHeader*>(ptr);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(LinearArenaHeader);
		header->next = nullptr;

		*arena = { ptr, size };

		return Result::SUCCESS;
	}

	Result init_atomic_linear_arena(MemoryArena *const arena, Allocator *const allocator, u64 const size) {
		if (size < sizeof(LinearArenaHeader)) {
			return Result::INVALID_ARGS;
		}

		auto block = allocator->allocate(size, 2048);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		auto header = static_cast<LinearArenaHeader*>(block);
		atomic_store(&header->allocationCount, 0);
		atomic_store(&header->requestedMemory, 0);
		atomic_store(&header->usedMemory, sizeof(LinearArenaHeader));
		header->next = nullptr;

		*arena = { block, size };

		return Result::SUCCESS;
	}

	void* allocate_from_linear_arena(MemoryArena *const arena, u64 const size, u64 const alignment) {
		if (size < 1 || alignment < 1) {
			return nullptr;
		}

		auto header = static_cast<LinearArenaHeader*>(arena->block);

		auto offset = align(header->usedMemory, alignment);
		auto nUsedMemory = offset + size;

		if (nUsedMemory < arena->size) {
			// If there was enough room for this allocation
			header->usedMemory = nUsedMemory;
			++header->allocationCount;
			header->requestedMemory += size;

			return add_ptr(arena->block, offset);
		}

		return nullptr;
	}

	void* allocate_from_atomic_linear_arena(MemoryArena *const arena, u64 const size, u64 const alignment) {
		if (size < 1 || alignment < 1) {
			return nullptr;
		}

		auto header = static_cast<LinearArenaHeader*>(arena->block);

		auto usedMemory = atomic_load(&header->usedMemory);
		u64 offset, nUsedMemory;

		do {
			offset = align(usedMemory + size, alignment);
			nUsedMemory = offset + size;
		} while (nUsedMemory <= arena->size
				&& !atomic_compare_exchange(&header->usedMemory, &usedMemory, nUsedMemory));

		if (nUsedMemory <= arena->size) {
			// If there was enough room for this allocation
			atomic_fetch_add(&header->allocationCount, u64{1});
			atomic_fetch_add(&header->requestedMemory, size);

			return add_ptr(arena->block, offset);
		}

		return nullptr;
	}

	void free_from_linear_arena(MemoryArena *arena, void *ptr, u64 size) {
	}

	void free_from_atomic_linear_arena(MemoryArena *arena, void *ptr, u64 size) {
	}

	Result copy_linear_arena(MemoryArena *const dst, MemoryArena *const src) {
		if (dst->size < src->size) {
			return Result::INVALID_ARGS;
		}

		auto srcHeader = static_cast<LinearArenaHeader*>(src->block);
		std::memcpy(dst->block, src->block, srcHeader->usedMemory);

		return Result::SUCCESS;
	}

	Result copy_atomic_linear_arena(MemoryArena *const dst, MemoryArena *const src) {
		if (dst->size < src->size) {
			return Result::INVALID_ARGS;
		}

		auto srcHeader = static_cast<LinearArenaHeader*>(src->block);
		std::memcpy(dst->block, src->block, srcHeader->usedMemory);

		return Result::SUCCESS;
	}

	void clear_linear_arena(MemoryArena *const arena) {
		auto header = static_cast<LinearArenaHeader*>(arena->block);

		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(LinearArenaHeader);
	}

	void clear_atomic_linear_arena(MemoryArena *const arena) {
		auto header = static_cast<LinearArenaHeader*>(arena->block);

		atomic_store(&header->allocationCount, 0);
		atomic_store(&header->requestedMemory, 0);
		atomic_store(&header->usedMemory, sizeof(LinearArenaHeader));
	}

namespace {

	static thread_local MTHeapBlockHeader *threadHeapList = nullptr;

	void reset_mt_heap_block(MTHeapBlockHeader *block) {
		block->next = nullptr;
		block->allocationCount = 0;
		block->requestedMemory = 0;
		block->usedMemory = sizeof(MTHeapBlockHeader);
	}

	MTHeapBlockHeader* allocate_mt_heap_block(MTHeapArenaHeader *header) {
		auto block = header->allocator->allocate(header->blockSize, 2048);
		if (!block)
			return nullptr;

		auto blockHeader = static_cast<MTHeapBlockHeader*>(block);
		reset_mt_heap_block(blockHeader);

		atomic_fetch_add(&header->totalBlocks, 1);
		atomic_fetch_add(&header->totalUsedMemory, blockHeader->usedMemory);

		return blockHeader;
	}

	void return_free_mt_heap_block(MTHeapArenaHeader *header, MTHeapBlockHeader *block) {
		atomic_fetch_add(&header->usedBlocks, -1);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		block->next = header->freeHeapList;
		header->freeHeapList = block;
	}

	MTHeapBlockHeader* grab_free_mt_heap_block(MTHeapArenaHeader *header) {
		atomic_fetch_add(&header->usedBlocks, 1);

		MTHeapBlockHeader *block = nullptr;
		{
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			block = header->freeHeapList;
			if (block)
				header->freeHeapList = block->next;
		}

		if (!block)
			block = allocate_mt_heap_block(header);

		return block;
	}

	void* allocate_from_mt_heap_block(
			MTHeapArenaHeader *header, MTHeapBlockHeader *block, u64 size, u64 alignment) {
		if (!block || !size || !alignment)
			return nullptr;

		assert(size < header->blockSize);

		auto offset = align(block->usedMemory, alignment);
		auto nUsedMemory = offset + size;

		if (nUsedMemory < header->blockSize) {
			atomic_fetch_add(&header->totalAllocationCount, 1);
			atomic_fetch_add(&header->totalRequestedMemory, size);
			atomic_fetch_add(&header->totalUsedMemory, nUsedMemory - block->usedMemory);

			block->allocationCount += 1;
			block->requestedMemory += size;
			block->usedMemory = nUsedMemory;

			return add_ptr(block, offset);
		}

		return nullptr;
	}

	bool free_from_mt_heap_block(MTHeapArenaHeader *header, MTHeapBlockHeader *block, void *ptr, u64 size) {
		if (!block)
			return false;

		if (ptr < block || ptr > add_ptr(block, header->blockSize))
			return false;

		assert(size < block->requestedMemory);

		auto offset = ptr_diff(ptr, block);
		auto nUsedMemory = block->usedMemory;

		if (offset + size == block->usedMemory)
			// ptr is at the top of the linear arena
			nUsedMemory = offset;

		atomic_fetch_add(&header->totalAllocationCount, -1);
		atomic_fetch_add(&header->totalRequestedMemory, -size);
		atomic_fetch_add(&header->totalUsedMemory, nUsedMemory - block->usedMemory);

		block->allocationCount -= 1;
		block->requestedMemory -= size;
		block->usedMemory = nUsedMemory;

		return block->requestedMemory == 0;
	}

}

	i32 init_heap_linear_arena(MemoryArena *arena, Allocator *allocator, u64 blockSize) {
		auto header = make<MTHeapArenaHeader>(allocator, 1);
		if (!header)
			return 2;

		header->allocator = allocator;
		header->blockSize = blockSize;
		header->totalUsedMemory = sizeof(MTHeapArenaHeader);

		*arena = { header, sizeof(MTHeapArenaHeader) };

		return 0;
	}

	void* allocate_from_heap_linear_arena(MemoryArena *arena, u64 size, u64 alignment) {
		if (!size || !alignment)
			return nullptr;

		assert(alignment <= 64);
		auto header = static_cast<MTHeapArenaHeader*>(arena->block);
		assert(size < header->blockSize - 64);

		auto result = allocate_from_mt_heap_block(header, threadHeapList, size, alignment);
		if (result)
			return result;

		// Failed to allocate memory from block, grab a new block
		auto nBlock = grab_free_mt_heap_block(header);
		nBlock->next = threadHeapList;
		threadHeapList = nBlock;

		return allocate_from_mt_heap_block(header, threadHeapList, size, alignment);
	}

	void free_from_heap_linear_arena(MemoryArena *arena, void *ptr, u64 size) {
		if (!ptr || !size) {
			assert(ptr == nullptr && size == 0);
			return;
		}

		auto header = static_cast<MTHeapArenaHeader*>(arena->block);
		assert(size < header->blockSize - 64);

		if (free_from_mt_heap_block(header, threadHeapList, ptr, size)) {
			// The block is empty so return it
			auto block = threadHeapList;
			threadHeapList = block->next;
			reset_mt_heap_block(block);
			return_free_mt_heap_block(header, block);
		}
	}

	void clear_heap_linear_arena(MemoryArena *arena) {
		auto header = static_cast<MTHeapArenaHeader*>(arena->block);

		while (threadHeapList) {
			auto block = threadHeapList;
			threadHeapList = block->next;

			atomic_fetch_add(&header->totalAllocationCount, -block->allocationCount);
			atomic_fetch_add(&header->totalRequestedMemory, -block->requestedMemory);
			atomic_fetch_add(&header->totalUsedMemory, -block->usedMemory + sizeof(MTHeapBlockHeader));

			reset_mt_heap_block(block);
			return_free_mt_heap_block(header, block);
		}
	}

	Result init_ring_arena(MemoryArena *const arena, Allocator *const allocator, u64 const size) {
		if (size < sizeof(RingArenaHeader)) {
			return Result::INVALID_ARGS;
		}

		auto block = allocator->allocate(size, 2048);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		auto header = static_cast<RingArenaHeader*>(block);
		header->offset = sizeof(RingArenaHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		header->usedMemory = sizeof(RingArenaHeader);
		header->next = nullptr;

		*arena = { block, size  };

		return Result::SUCCESS;
	}

	void* allocate_from_ring_arena(MemoryArena *const arena, u64 const size, u64 const alignment) {
		if (size < 1 || alignment < 1) {
			return nullptr;
		}

		auto header = static_cast<RingArenaHeader*>(arena->block);

		// Try to fit allocation at end of buffer
		auto ao = align_offset_with_header(u64{ header->offset }, alignment, sizeof(u32));
		auto alignedOffset = header->offset + ao;
		auto padding = ao;
		auto usedMemory = header->usedMemory + padding;

		if (alignedOffset + size > arena->size) {
			// Wrap buffer if allocation doesn't fit at end
			ao = align_offset_with_header(u64{ sizeof(RingArenaHeader) }, alignment, sizeof(u32));
			alignedOffset = sizeof(RingArenaHeader) + ao;
			padding = ao + arena->size - header->offset;
			usedMemory = header->usedMemory + padding;
		}

		if (usedMemory + size > arena->size) {
			// Out of memory
			return nullptr;
		}

		auto start = add_ptr(arena->block, alignedOffset);

		// Place allocation header
		*static_cast<u32*>(sub_ptr(start, sizeof(u32))) = padding;

		usedMemory += size;

		header->offset = alignedOffset + size;
		header->usedMemory = usedMemory;
		header->requestedMemory += size;
		++header->allocationCount;

		return start;
	}

	void deallocate_from_ring_arena(MemoryArena *const arena, void *const ptr, u64 const size) {
		auto header = static_cast<RingArenaHeader*>(arena->block);

		auto padding = *static_cast<u32*>(sub_ptr(ptr, sizeof(u32)));

		header->usedMemory -= (size + padding);
		header->requestedMemory -= size;
		--header->allocationCount;
	}

	void clear_ring_arena(MemoryArena *const arena) {
		auto header = static_cast<RingArenaHeader*>(arena->block);
		header->offset = sizeof(RingArenaHeader);
		header->usedMemory = sizeof(RingArenaHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;
	}

	void destroy_arena(MemoryArena *const arena, Allocator *const allocator) {
		allocator->deallocate(arena->block, arena->size);
		*arena = {};
	}

	bool arena_contains(MemoryArena *arena, void *ptr) {
		auto start = reinterpret_cast<u64>(arena->block);
		auto end = reinterpret_cast<u64>(add_ptr(arena->block, arena->size));
		auto p = reinterpret_cast<u64>(ptr);

		return start < p && p <= end;
	}

	void* push_stack(MemoryArena *arena) {
		auto arenaHeader = static_cast<LinearArenaHeader*>(arena->block);
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
		auto arenaHeader = static_cast<LinearArenaHeader*>(arena->block);
		auto stackHeader = static_cast<StackHeader*>(sub_ptr(stackPtr, sizeof(StackHeader)));
		auto ogUsedMemory = reinterpret_cast<u64>(stackPtr) - reinterpret_cast<u64>(arena->block) - sizeof(StackHeader);
		arenaHeader->usedMemory = ogUsedMemory;
		arenaHeader->allocationCount = stackHeader->allocationCount;
		arenaHeader->requestedMemory = stackHeader->requestedMemory;
	}

	Result init_memory_pool(MemoryArena *arena, Allocator *allocator, u64 size, u64 alignment) {
		auto const poolSize = ensure_pow2(size);
		auto const totalSize = poolSize + align(sizeof(PoolHeader), alignment);

		auto block = allocator->allocate(totalSize, alignment);
		if (!block) {
			return Result::OUT_OF_MEMORY;
		}

		*arena = {
			block,
			totalSize
		};

		auto poolNode = static_cast<PoolNode*>(add_ptr(block, align(sizeof(PoolHeader), alignment)));
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

		auto targetNodeSize = ensure_pow2(align(size, poolHeader->alignment));
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
						align(rfn, firstNode->size + secondNode->size) == rfn) {
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
		auto nodeSize = ensure_pow2(align(size, poolHeader->alignment));
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

	void* std_aligned_alloc_wrapper(MemoryArena*, u64 size, u64 alignment) {
#ifdef _WIN32
		return _aligned_malloc(size, alignment);
#else
		return aligned_alloc(alignment, align(size, alignment));
#endif
	}

	void* std_aligned_alloc_garbage_wrapper(MemoryArena*, u64 size, u64 alignment) {
#ifdef _WIN32
		auto result = _aligned_malloc(size, alignment);
#else
		auto result = aligned_alloc(alignment, align(size, alignment));
#endif
		std::memset(result, 0x99, align(size, alignment));
		return result;
	}

	void std_free_wrapper(MemoryArena*, void *ptr, u64) {
#ifdef _WIN32
		_aligned_free(ptr);
#else
		free(ptr);
#endif
	}

	void std_clear_wrapper(MemoryArena*) {
	}

	Allocator* globalAllocator;
	Allocator* temporaryAllocator;

}

