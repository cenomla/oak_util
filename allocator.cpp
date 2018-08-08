#include "allocator.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "fmt.h"
#include "ptr.h"

namespace oak {

	IAllocator::~IAllocator() {}

	void ProxyAllocator::destroy() {
		//free memory
		if (numAllocs > 0) {
			print_fmt("memory leak, remaining blocks: %\n", numAllocs);
		}

		MemBlock *p = memList;
		MemBlock *next = nullptr;
		while (p != nullptr) {
			next = static_cast<MemBlock*>(p->next);
			::free(p);
			p = next;
			numAllocs --;
		}

		assert(numAllocs == 0);
	}

	void* ProxyAllocator::alloc(size_t size) {
		void *ptr = aligned_alloc(64, size + sizeof(MemBlock));
		MemBlock *l = static_cast<MemBlock*>(ptr);
		l->next = memList;
		l->size = size;
		memList = l;

		numAllocs ++;

		return ptr::add(ptr, sizeof(MemBlock));
	}

	void ProxyAllocator::free(const void *ptr, size_t size) {
		//search through linked list for ptr - sizeof(memList) then remove from list and deallocate
		auto memBlock = static_cast<const MemBlock*>(ptr::subtract(ptr, sizeof(MemBlock)));

		MemBlock *p = memList;
		MemBlock *prev = nullptr;
		while (p != nullptr) {
			if (p == memBlock) {
				if (prev != nullptr) {
					prev->next = p->next;
				} else {
					memList = static_cast<MemBlock*>(p->next);
				}
				numAllocs--;
				::free(p);
				break;
			}
			prev = p;
			p = static_cast<MemBlock*>(p->next);
		}
	}

	bool ProxyAllocator::contains(const void *ptr) {
		//search through memblocks and check each if they contain ptr
		auto uintptr = reinterpret_cast<uintptr_t>(ptr);
		MemBlock *p = memList;
		while (p) {
			auto uintp = reinterpret_cast<uintptr_t>(p);
			if (uintptr > uintp && uintptr < uintp + p->size) {
				return true;
			}
			p = static_cast<MemBlock*>(p->next);
		}
		return false;
	}

	LinearAllocator::LinearAllocator(size_t pageSize_, size_t alignment_, IAllocator *parent_) :
		pageSize{ pageSize_ }, alignment{ alignment_ }, parent{ parent_ } {}

	void LinearAllocator::init() {
		//we need a page size and parent to continue
		assert(pageSize > sizeof(MemBlock));
		assert(parent);
		//allocate first page
		size_t totalPageSize = pageSize + sizeof(MemBlock);
		start = parent->alloc(totalPageSize);
		assert(start);
		//fill in page header
		MemBlock *header = static_cast<MemBlock*>(start);
		header->next = nullptr;
		header->size = totalPageSize;
		//fill in the rest of the struct
		pagePtr = start;
		pos = ptr::add(start, sizeof(MemBlock));
	}

	void LinearAllocator::destroy() {
		//deallocate used memory
		MemBlock *prev = nullptr;
		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			prev = p;
			p = static_cast<MemBlock*>(p->next);
			parent->free(prev, prev->size);
		}
		//make sure we don't have pointers to invalid memory
		start = nullptr;
		pagePtr = nullptr;
		pos = nullptr;
	}


	void* LinearAllocator::alloc(size_t size) {
		assert(size != 0 && size <= pageSize);

		uint32_t adjustment = ptr::align_offset(pos, alignment);
		uintptr_t alignedAddress = reinterpret_cast<uintptr_t>(pos) + adjustment;
		MemBlock *header = static_cast<MemBlock*>(pagePtr);

		if (alignedAddress - reinterpret_cast<uintptr_t>(pagePtr) + size > header->size) {
			if (header->next == nullptr) {
				grow();
			}
			pagePtr = header->next;
			pos = ptr::add(pagePtr, sizeof(MemBlock));
			adjustment = ptr::align_offset(pos, alignment);
			alignedAddress = reinterpret_cast<uintptr_t>(pos) + adjustment;
		}

		pos = reinterpret_cast<void*>(alignedAddress + size);
		return reinterpret_cast<void*>(alignedAddress);
	}

	void LinearAllocator::free(const void *ptr, size_t size) {}

	bool LinearAllocator::contains(const void *ptr) {
		auto uintptr = reinterpret_cast<uintptr_t>(ptr);

		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			auto uintp = reinterpret_cast<uintptr_t>(p);
			if (uintptr > uintp && uintptr < uintp + p->size) {
				return true;
			}
			p = static_cast<MemBlock*>(p->next);
		}
		return false;
	}

	void LinearAllocator::clear() {
		pagePtr = start;
		pos = ptr::add(pagePtr, sizeof(MemBlock));
	}

	void LinearAllocator::grow() {
		size_t totalPageSize = pageSize + sizeof(MemBlock);
		void *ptr = parent->alloc(totalPageSize);
		assert(ptr != nullptr);
		MemBlock *header = static_cast<MemBlock*>(pagePtr);
		header->next = ptr;
		MemBlock *nHeader = static_cast<MemBlock*>(ptr);
		nHeader->next = nullptr;
		nHeader->size = totalPageSize;
	}

	FreelistAllocator::FreelistAllocator(size_t pageSize_, size_t alignment_, IAllocator *parent_) :
		pageSize{ pageSize_ }, alignment{ alignment_ }, parent{ parent_ } {}

	void FreelistAllocator::init() {
		assert(pageSize > sizeof(MemBlock));
		assert(parent);
		size_t totalPageSize = pageSize + sizeof(MemBlock);
		start = parent->alloc(totalPageSize);
		assert(start);
		MemBlock *header = static_cast<MemBlock*>(start);
		header->next = nullptr;
		header->size = totalPageSize;

		freeList = header + 1;
		freeList->size = pageSize;
		freeList->next = nullptr;
	}

	void FreelistAllocator::destroy() {
		MemBlock *prev = nullptr;
		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			prev = p;
			p = static_cast<MemBlock*>(p->next);
			parent->free(prev, prev->size);
		}
	}

	void* FreelistAllocator::alloc(size_t size) {
		assert(size != 0 && size <= pageSize);
		MemBlock *prev = nullptr;
		MemBlock *p = freeList;

		while (p) {
			//Calculate adjustment needed to keep object correctly aligned
			uint32_t adjustment = ptr::align_offset_with_header(p, alignment, sizeof(AllocationHeader));

			size_t totalSize = size + adjustment;

			//If allocation doesn't fit in this FreeBlock, try the next
			if (p->size < totalSize) {
				prev = p;
				p = static_cast<MemBlock*>(p->next);
				if (p->next == nullptr) {
					grow(p);
				}
				continue;
			}

			//If allocations in the remaining memory will be impossible
			if (p->size - totalSize <= sizeof(AllocationHeader)) {
				//Increase allocation size instead of creating a new FreeBlock
				totalSize = p->size;

				if (prev != nullptr) {
					prev->next = p->next;
				} else {
					freeList = static_cast<MemBlock*>(p->next);
				}
			} else {
				//else create a new FreeBlock containing remaining memory
				MemBlock* next = static_cast<MemBlock*>(ptr::add(p, totalSize));
				next->size = p->size - totalSize;
				next->next = p->next;

				if (prev != nullptr) {
					prev->next = next;
				}
				else {
					freeList = next;
				}
			}

			uintptr_t alignedAddress = reinterpret_cast<uintptr_t>(p) + adjustment;

			AllocationHeader* header = reinterpret_cast<AllocationHeader*>(alignedAddress - sizeof(AllocationHeader));
			header->size = totalSize;
			header->adjustment = adjustment;

			assert(ptr::align_offset(reinterpret_cast<void*>(alignedAddress), alignment) == 0);

			return reinterpret_cast<void*>(alignedAddress);
		}

		return nullptr;
	}

	void FreelistAllocator::free(const void *ptr, size_t size) {
		assert(ptr != nullptr);

		const AllocationHeader* header = static_cast<const AllocationHeader*>(ptr::subtract(ptr, sizeof(AllocationHeader)));

		uintptr_t   blockStart = reinterpret_cast<uintptr_t>(ptr) - header->adjustment;
		size_t blockSize = header->size;
		uintptr_t   blockEnd = blockStart + blockSize;

		MemBlock *prev = nullptr;
		MemBlock *p = freeList;

		while (p) {
			if (reinterpret_cast<uintptr_t>(p) >= blockEnd) {
				break;
			}

			prev = p;
			p = static_cast<MemBlock*>(p->next);
		}

		if (prev == nullptr) {
			prev = reinterpret_cast<MemBlock*>(blockStart);
			prev->size = blockSize;
			prev->next = freeList;

			freeList = prev;
		} else if (reinterpret_cast<uintptr_t>(prev) + prev->size == blockStart) {
			prev->size += blockSize;
		} else {
			MemBlock *temp = reinterpret_cast<MemBlock*>(blockStart);
			temp->size = blockSize;
			temp->next = prev->next;
			prev->next = temp;

			prev = temp;
		}

		if (p != nullptr && reinterpret_cast<uintptr_t>(p) == blockEnd) {
			prev->size += p->size;
			prev->next = p->next;
		}
	}

	bool FreelistAllocator::contains(const void *ptr) {
		auto uintptr = reinterpret_cast<uintptr_t>(ptr);

		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			auto uintp = reinterpret_cast<uintptr_t>(p);
			if (uintptr > uintp && uintptr < uintp + p->size) {
				return true;
			}
			p = static_cast<MemBlock*>(p->next);
		}
		return false;
	}

	void FreelistAllocator::grow(MemBlock *lastNode) {

		//find the end of the used block list
		MemBlock *prevHeader = nullptr;
		MemBlock *header = static_cast<MemBlock*>(start);

		while (header) {
			prevHeader = header;
			header = static_cast<MemBlock*>(header->next);
		}

		//after loop prev = end of free list

		//create new memory block and append it to the used block list
		void *ptr = parent->alloc(pageSize + sizeof(MemBlock));
		assert(ptr != nullptr);
		prevHeader->next = ptr;
		header = static_cast<MemBlock*>(ptr);
		header->next = nullptr;
		header->size = pageSize + sizeof(MemBlock);

		//create new free block with the added size
		MemBlock *newBlock = header + 1;
		newBlock->size = pageSize;
		newBlock->next = nullptr;

		lastNode->next = newBlock;
	}

	PoolAllocator::PoolAllocator(IAllocator *parent_) : parent{ parent_ } {}

	void PoolAllocator::init() {
		//allocate pools array and fill with zeros
		pools = static_cast<Pool*>(parent->alloc(POOL_COUNT * sizeof(Pool)));
		std::memset(pools, 0, POOL_COUNT * sizeof(Pool));
	}

	void PoolAllocator::destroy() {
		assert(pools);
		//deallocate all pools
		for (int32_t i = 0; i < POOL_COUNT; i++) {
			auto block = static_cast<void**>(pools[i].firstBlock);
			auto allocationSize = (1ll << (i + POOL_FIRST_INDEX));
			auto allocationCount = POOL_INITIAL_ALLOCATION_COUNT;
			//deallocate each block
			while (block) {
				auto p = block;
				block = static_cast<void**>(*block);
				parent->free(p, sizeof(void*) + allocationSize * allocationCount);
				allocationCount *= 2;
				pools[i].blockCount--;
			}
			assert(pools[i].blockCount == 0);
		}
		//deallocate the pool array
		parent->free(pools, sizeof(Pool) * POOL_COUNT);
		pools = nullptr;
	}

	void* PoolAllocator::alloc(size_t size) {
		int32_t idx = blog2(size) - POOL_FIRST_INDEX;
		if (idx < POOL_COUNT) {
			if (!pools[idx].freelist) {
				grow_pool(idx);
			}

			void *p = pools[idx].freelist;
			pools[idx].freelist = static_cast<void**>(*pools[idx].freelist);

			return p;
		} else {
			//fallback to parent allocator
			return parent->alloc(size);
		}
	}

	void PoolAllocator::free(const void *ptr, size_t size) {
		int32_t idx = blog2(size) - POOL_FIRST_INDEX;
		if (idx < POOL_COUNT) {
			auto p = const_cast<void*>(ptr);
			*static_cast<void**>(p) = pools[idx].freelist;
			pools[idx].freelist = static_cast<void**>(p);
		} else {
			parent->free(ptr, size);
		}
	}

	bool PoolAllocator::contains(const void *ptr) {
		return false;
	}

	void PoolAllocator::grow_pool(int32_t idx) {
		assert(!pools[idx].freelist);

		//allocate a new block
		auto allocationSize = (1ll << (idx + POOL_FIRST_INDEX));
		auto allocationCount = POOL_INITIAL_ALLOCATION_COUNT * (1ll << pools[idx].blockCount);
		auto block = parent->alloc(sizeof(void*) + allocationSize * allocationCount);
		//first 8 bytes of the block is the next pointer so set it to nullptr
		*static_cast<void**>(block) = nullptr;
		//create a freelist out of the block
		pools[idx].freelist = static_cast<void**>(ptr::add(block, sizeof(void*)));

		auto p = pools[idx].freelist;

		for (int32_t i = 0; i < allocationCount - 1; i++) {
			*p = ptr::add(p, allocationSize);
			p = static_cast<void**>(*p);
		}

		*p = nullptr;

		//update block info
		if (!pools[idx].firstBlock) {
			pools[idx].firstBlock = block;
		}
		pools[idx].lastBlock = block;
		pools[idx].blockCount++;
	}

	FixedPoolAllocator::FixedPoolAllocator(
			size_t pageSize_, size_t objectSize_, size_t alignment_, IAllocator *parent_) :
		pageSize{ pageSize_ }, objectSize{ objectSize_ }, alignment{ alignment_ }, parent{ parent_ } {}

	void FixedPoolAllocator::init() {
		assert(pageSize > sizeof(MemBlock));
		assert(parent);
		size_t totalPageSize = pageSize + sizeof(MemBlock);
		start = parent->alloc(totalPageSize);
		assert(start);
		MemBlock *header = static_cast<MemBlock*>(start);
		header->next = nullptr;
		header->size = totalPageSize;

		//make sure the size is aligned
		objectSize = ptr::align_size(objectSize, alignment);

		freeList = static_cast<void**>(ptr::add(start, sizeof(MemBlock)));

		void **p = freeList;

		for (size_t i = 0; i < count() - 1; i++) {
			*p = ptr::add(p, objectSize);
			p = static_cast<void**>(*p);
		}

		*p = nullptr;
	}

	void FixedPoolAllocator::destroy() {
		MemBlock *prev = nullptr;
		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			prev = p;
			p = static_cast<MemBlock*>(p->next);
			parent->free(prev, prev->size);
		}
	}

	void* FixedPoolAllocator::alloc(size_t size) {
		if (*freeList == nullptr) {
			grow();
		}

		void *p = freeList;
		freeList = static_cast<void**>(*freeList);

		return p;
	}

	void FixedPoolAllocator::free(const void *p, size_t size) {
		auto ptr = const_cast<void*>(p);
		*static_cast<void**>(ptr) = freeList;
		freeList = static_cast<void**>(ptr);
	}

	bool FixedPoolAllocator::contains(const void *ptr) {
		auto uintptr = reinterpret_cast<uintptr_t>(ptr);

		MemBlock *p = static_cast<MemBlock*>(start);
		while (p) {
			auto uintp = reinterpret_cast<uintptr_t>(p);
			if (uintptr > uintp && uintptr < uintp + p->size) {
				return true;
			}
			p = static_cast<MemBlock*>(p->next);
		}
		return false;
	}

	void FixedPoolAllocator::grow() {
		//find end of freeList
		void **prev = nullptr;
		void **p = freeList;

		while (p) {
			prev = p;
			p = static_cast<void**>(*p);
		}

		//find the end of the used block list
		MemBlock *prevHeader = nullptr;
		MemBlock *header = static_cast<MemBlock*>(start);

		while (header) {
			prevHeader = header;
			header = static_cast<MemBlock*>(header->next);
		}

		//after loop prev = end of free list

		//create new memory block and append it to the used block list
		size_t totalPageSize = pageSize + sizeof(MemBlock);
		void *ptr = parent->alloc(totalPageSize);
		assert(ptr != nullptr);
		prevHeader->next = ptr;
		header = static_cast<MemBlock*>(ptr);
		header->next = nullptr;
		header->size = totalPageSize;

		*prev = ptr::add(ptr, sizeof(MemBlock));

		p = static_cast<void**>(*prev);

		for (size_t i = 0; i < count() - 1; i++) {
			*p = ptr::add(p, objectSize);
			p = static_cast<void**>(*p);
		}

		*p = nullptr;
	}

	size_t FixedPoolAllocator::count() {
		return (pageSize & ~(alignment-1)) / objectSize;
	}

	ProxyAllocator proxyAlloc;
}
