#pragma once

#include <cstddef>
#include <cinttypes>

#include "osig_defs.h"

namespace oak {

	struct MemBlock {
		void *next;
		size_t size;
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

		LinearAllocator(size_t a, size_t b, IAllocator *c);

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

		FreelistAllocator(size_t a, size_t b, IAllocator *c);

		void init();
		void destroy();

		void* alloc(size_t size) override;
		void free(const void *ptr, size_t size) override;
		bool contains(const void *ptr) override;
		void grow(MemBlock *lastNode);
	};

	struct PoolAllocator : IAllocator {
		size_t pageSize = 0;
		size_t objectSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start;
		void **freeList = nullptr;

		PoolAllocator(size_t a, size_t b, size_t c, IAllocator *d);

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
