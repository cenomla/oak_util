#pragma once

#include <cstddef>
#include <cinttypes>

namespace oak {

	struct MemBlock {
		void *next;
		size_t size;
	};

	struct IAllocator {
		virtual void* allocate(size_t size) = 0;
		virtual void deallocate(void *ptr, size_t size) = 0;
	protected:
		virtual ~IAllocator();
	};

	struct ProxyAllocator : IAllocator {
		MemBlock *memList = nullptr;
		size_t numAllocs = 0;

		void destroy();
		void* allocate(size_t size) override;
		void deallocate(void *ptr, size_t size) override;

	};	

	struct LinearAllocator : IAllocator {
		size_t pageSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start = nullptr;
		void *pagePtr = nullptr;
		void *pos = nullptr;

		LinearAllocator(size_t a, size_t b, IAllocator *c) :
			pageSize{ a }, alignment{ b }, parent{ c } {}

		void init();
		void destroy();
		void* allocate(size_t size) override;
		void deallocate(void *ptr, size_t size) override;
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

		FreelistAllocator(size_t a, size_t b, IAllocator *c) : 
			pageSize{ a }, alignment{ b }, parent{ c } {}

		void init();
		void destroy();

		void* allocate(size_t size) override;
		void deallocate(void *ptr, size_t size) override;
		void grow(MemBlock *lastNode);
	};

	struct PoolAllocator : IAllocator {
		size_t pageSize = 0;
		size_t objectSize = 0;
		size_t alignment = 8;
		IAllocator *parent = nullptr;
		void *start;
		void **freeList = nullptr;

		PoolAllocator(size_t a, size_t b, size_t c, IAllocator *d) :
			pageSize{ a }, objectSize{ b }, alignment{ c }, parent{ d } {}

		void init();
		void destroy();
		void* allocate(size_t size) override;
		void deallocate(void *ptr, size_t size) override;
		void grow();
		size_t count();
	};

	template<typename T>
	void* allocate(T *alloc, size_t size);
	template<typename T>
	void deallocate(T *alloc, void *ptr, size_t size);

	struct Allocator {
		void *data;
		void* (*fpalloc)(void*, size_t);
		void (*fpfree)(void*, void*, size_t);

		template<typename T>
		Allocator(T *d) : data{ d }, fpalloc{ allocate<T> }, fpfree{ deallocate<T> } {}

		void* alloc(size_t size);
		void free(void *ptr, size_t size);
	};

	extern ProxyAllocator proxyAlloc;
	extern FreelistAllocator listAlloc;
	extern LinearAllocator frameAlloc;

	void init_allocators();
	void destroy_allocators();

}
