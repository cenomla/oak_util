#pragma once

#include <cstddef>
#include <cinttypes>

#include "osig_defs.h"
#include "bit.h"

namespace oak {

	struct MemBlock {
		void *next = nullptr;
		size_t size = 0;
	};

	struct _reflect(oak::catagory::none) IAllocator {
		virtual void *alloc(size_t size) = 0;
		virtual void free(const void *ptr, size_t size) = 0;
		virtual bool contains(const void *ptr) = 0;
		virtual ~IAllocator();
	protected:
		IAllocator() = default;
	};

	struct ProxyAllocator : IAllocator {
		MemBlock *memList = nullptr;
		size_t numAllocs = 0;

		void destroy();
		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;

	};

	struct LinearAllocator : IAllocator {
		size_t pageSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start = nullptr;
		void *pagePtr = nullptr;
		void *pos = nullptr;

		LinearAllocator(size_t pageSize_, size_t alignment_, IAllocator *parent_);

		void init();
		void destroy();
		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;
		void clear();
		void grow();
	};

	struct FreelistAllocator : IAllocator {
		struct AllocationHeader {
			size_t size;
			uint32_t adjustment;
		};

		size_t pageSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start = nullptr;
		MemBlock *freeList = nullptr;

		FreelistAllocator(size_t pageSize_, size_t alignment_, IAllocator *parent_);

		void init();
		void destroy();

		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;
		void grow(MemBlock *lastNode);
	};

	struct PoolAllocator : IAllocator {
		static constexpr int64_t MIN_POOL_SIZE = 4; //size in bytes of smallest pool
		static constexpr int64_t MAX_POOL_SIZE = 4096; //size in bytes of largest pool
		static constexpr int64_t POOL_INITIAL_ALLOCATION_COUNT = 32;
		static constexpr int64_t POOL_COUNT = blog2(MAX_POOL_SIZE) - blog2(MIN_POOL_SIZE);
		static constexpr int64_t POOL_FIRST_INDEX = blog2(MIN_POOL_SIZE);

		struct Pool {
			void *firstBlock = nullptr;
			void *lastBlock = nullptr;
			int64_t blockCount = 0;
			void **freeList = nullptr;
		};

		IAllocator *parent = nullptr;
		int32_t alignment = 8; //alignment of allocations
		Pool *pools = nullptr; //stores an array of pools

		PoolAllocator(IAllocator *parent_);

		void init();
		void destroy();

		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;
		void grow_pool(int32_t idx);
	};

	//Will become deprecated in favor of PoolArrays and a multi sized pool allocator
	struct FixedPoolAllocator : IAllocator {
		size_t pageSize = 0;
		size_t objectSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start;
		void **freeList = nullptr;

		FixedPoolAllocator(size_t pageSize_, size_t objectSize_, size_t alignment_, IAllocator *parent_);

		void init();
		void destroy();
		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;
		void grow();
		size_t count();
	};

	extern ProxyAllocator proxyAlloc;
}
