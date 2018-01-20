#pragma once

#include <cstddef>
#include <cinttypes>

#include "osig_defs.h"

namespace oak {

	struct MemBlock {
		void *next;
		size_t size;
	};

	template<typename T>
	void* falloc(void *data, size_t size) {
		return static_cast<T*>(data)->alloc(size);
	}

	template<typename T>
	void ffree(void *data, const void *ptr, size_t size) {
		static_cast<T*>(data)->free(ptr, size);
	}

	struct _reflect("util") Allocator {
		void *data;
		void* (*fpalloc)(void*, size_t) _exclude;
		void (*fpfree)(void*, const void*, size_t) _exclude;

		template<typename T>
		Allocator(T *d) : data{ d }, fpalloc{ falloc<T> }, fpfree{ ffree<T> } {}

		void* alloc(size_t size);
		void free(const void *ptr, size_t size);
	};

	struct ProxyAllocator : Allocator {
		MemBlock *memList = nullptr;
		size_t numAllocs = 0;

		ProxyAllocator() : Allocator{ this } {}

		void destroy();
		void* alloc(size_t size);
		void free(const void *ptr, size_t size);

	};	

	struct LinearAllocator : Allocator {
		size_t pageSize = 0;
		size_t alignment = 8;
		Allocator *parent = nullptr;
		void *start = nullptr;
		void *pagePtr = nullptr;
		void *pos = nullptr;

		LinearAllocator(size_t a, size_t b, Allocator *c) :
			Allocator{ this }, pageSize{ a }, alignment{ b }, parent{ c } {}

		void init();
		void destroy();
		void* alloc(size_t size);
		void free(const void *ptr, size_t size);
		void clear();
		void grow();
	};

	struct FreelistAllocator : Allocator {
		struct AllocationHeader {
			size_t size;
			uint32_t adjustment;
		};

		size_t pageSize = 0;
		size_t alignment = 8;
		Allocator *parent = nullptr;
		void *start = nullptr;
		MemBlock *freeList = nullptr;

		FreelistAllocator(size_t a, size_t b, Allocator *c) : 
			Allocator{ this }, pageSize{ a }, alignment{ b }, parent{ c } {}

		void init();
		void destroy();

		void* alloc(size_t size);
		void free(const void *ptr, size_t size);
		void grow(MemBlock *lastNode);
	};

	struct PoolAllocator : Allocator {
		size_t pageSize = 0;
		size_t objectSize = 0;
		size_t alignment = 8;
		Allocator *parent = nullptr;
		void *start;
		void **freeList = nullptr;

		PoolAllocator(size_t a, size_t b, size_t c, Allocator *d) :
			Allocator{ this }, pageSize{ a },
			objectSize{ b }, alignment{ c }, parent{ d } {}

		void init();
		void destroy();
		void* alloc(size_t size);
		void free(const void *ptr, size_t size);
		void grow();
		size_t count();
	};

	extern ProxyAllocator proxyAlloc;
}
