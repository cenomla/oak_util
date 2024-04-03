#define OAK_UTIL_EXPORT_SYMBOLS
#include <oak_util/memory.h>

#ifdef _WIN32

#if defined(__SANITIZE_ADDRESS__)
#define HAS_ASAN 1
#else
#define HAS_ASAN 0
#endif // defined(__SANITIZE_ADDRESS__)

#else
#include <sanitizer/asan_interface.h>

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define HAS_ASAN 1
#else
#define HAS_ASAN 0
#endif // __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)

#endif // _WIN32

#if HAS_ASAN
#	define ASAN_RED_ZONE_SIZE 8
#else
#	define ASAN_RED_ZONE_SIZE 0
#endif

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif // _WIN32

#include <oak_util/atomic.h>
#include <oak_util/types.h>
#include <oak_util/ptr.h>
#include <oak_util/bit.h>

namespace oak {

namespace {

	static thread_local MemoryArena *_threadLocalArena = nullptr;

	usize _get_page_size() {
#ifdef _WIN32
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwPageSize;
#else
		auto scResult = sysconf(_SC_PAGE_SIZE);
		if (scResult == -1)
			return 1;
		return static_cast<usize>(scResult);
#endif
	}

	u64 _get_thread_id() {
#ifdef _WIN32
		return GetCurrentThreadId();
#else
		return static_cast<u64>(pthread_self());
#endif
	}

	void* _virtual_alloc_with_header(usize size, [[maybe_unused]] usize headerSize, usize *pageSize_) {
		auto pageSize = _get_page_size();
		assert(pageSize >= headerSize);
		if (pageSize_)
			*pageSize_ = pageSize;
		size = align(size, pageSize);
		auto addr = virtual_alloc(size);
		if (!addr)
			return nullptr;

		// Allocate the header
		if (commit_region(addr, pageSize) != 0) {
			virtual_free(addr, size);
			return nullptr;
		}
#if HAS_ASAN
		__asan_unpoison_memory_region(addr, headerSize);
#endif

		return addr;
	}

	i32 _memory_arena_ensure_commit_size(MemoryArenaHeader *header, usize nUsedMemory) {
		if (nUsedMemory > header->commitSize) {
			auto nCommitSize = ensure_pow2(nUsedMemory);
			if (nCommitSize > header->capacity)
				nCommitSize = header->capacity;
			if (commit_region(add_ptr(header, header->commitSize), nCommitSize - header->commitSize) != 0)
				return 1;
			header->commitSize = nCommitSize;
		}
		return 0;
	}

	MemoryArena* _require_thread_local_arena(MTMemoryArenaHeader *header) {
		if (_threadLocalArena)
			return _threadLocalArena;

		auto threadId = _get_thread_id();
		{

			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			auto it = &header->first;
			while (*it != nullptr) {
				if (bit_cast<MemoryArenaHeader*>(*it)->_threadId == threadId) {
					_threadLocalArena = *it;
					return _threadLocalArena;
				}
				it = &(bit_cast<MemoryArenaHeader*>(*it)->_nextArena);
			}
		}

		if (memory_arena_init(&_threadLocalArena, header->threadArenaSize) != 0)
			return nullptr;

		bit_cast<MemoryArenaHeader*>(_threadLocalArena)->_threadId = threadId;

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		if (header->last)
			bit_cast<MemoryArenaHeader*>(header->last)->_nextArena = _threadLocalArena;
		header->last = _threadLocalArena;
		if (!header->first)
			header->first = header->last;

		return _threadLocalArena;
	}

	isize _memory_heap_pool_idx(
			usize *objectSize,
			MemoryHeapHeader *heapHeader,
			usize size,
			[[maybe_unused]] usize alignment) {
		auto pow2Size = ensure_pow2(size);
		if (pow2Size < heapHeader->minPoolObjectSize)
			pow2Size = heapHeader->minPoolObjectSize;
		*objectSize = pow2Size;

		assert(alignment <= pow2Size);

		isize poolIdxOffset = blog2(heapHeader->minPoolObjectSize);
		isize maxPoolIdx = blog2(heapHeader->maxPoolObjectSize) - poolIdxOffset;
		isize poolIdx = blog2(pow2Size) - poolIdxOffset;

		assert(poolIdx >= 0);
		if (poolIdx > maxPoolIdx)
			poolIdx = -1;

		return poolIdx;
	}

	void* _memory_heap_alloc_pages(MemoryArenaHeader *header, usize size) {
		auto offset = align(header->usedMemory, header->pageSize);
		auto alignedSize = align(size, header->pageSize);
		if (offset + alignedSize > header->capacity)
			return nullptr;

		if (_memory_arena_ensure_commit_size(header, offset + alignedSize) != 0)
			return nullptr;

		header->usedMemory = offset + alignedSize;
#if HAS_ASAN
		__asan_unpoison_memory_region(add_ptr(header, offset), alignedSize);
#endif

		return add_ptr(header, offset);
	}

}

	void* virtual_alloc(usize size) {
#ifdef _WIN32
		return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
#else
		auto result = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (result == MAP_FAILED)
			return nullptr;

		return result;
#endif // _WIN32
	}

	bool virtual_try_grow(void *addr, [[maybe_unused]] usize size, usize nSize) {
#ifdef _WIN32
		auto result = VirtualAlloc(addr, nSize, MEM_RESERVE, PAGE_READWRITE);
		if (result == nullptr)
			return false;
		if (result != addr) {
			VirtualFree(result, 0, MEM_RELEASE);
			return false;
		}

		return true;
#else
		auto nAddr = add_ptr(addr, size);
		auto result = mmap(nAddr, nSize - size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (result == MAP_FAILED)
			return false;
		if (result != nAddr) {
			munmap(result, nSize - size);
			return false;
		}

		return true;
#endif // _WIN32
	}

	void virtual_free(void *addr, usize size) {
#ifdef _WIN32
		(void)size;
		auto result = VirtualFree(addr, 0, MEM_RELEASE);
		assert(result);
#else
		[[maybe_unused]] auto result = munmap(addr, size);
		assert(result == 0);
#endif // _WIN32
	}

	i32 commit_region(void *addr, usize size) {
#ifdef _WIN32
		auto result = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
		if (result == nullptr)
			return 1;
#if HAS_ASAN
		__asan_poison_memory_region(addr, size);
#endif
		return 0;
#else
		if (mprotect(addr, size, PROT_READ|PROT_WRITE) == -1)
			return 1;
		ASAN_POISON_MEMORY_REGION(addr, size);
		return 0;
#endif
	}

	i32 decommit_region(void *addr, usize size) {
#ifdef _WIN32
#if HAS_ASAN
		__asan_unpoison_memory_region(addr, size);
#endif
		if (VirtualFree(addr, size, MEM_DECOMMIT) == 0)
			return 1;
		return 0;
#else
		ASAN_UNPOISON_MEMORY_REGION(addr, size);
		if (mprotect(addr, size, PROT_NONE) == -1)
			return 1;
		if (madvise(addr, size, MADV_DONTNEED) == -1)
			return 1;
		return 0;
#endif
	}

	i32 memory_arena_init(MemoryArena **arena, usize size) {
		usize pageSize;
		auto addr = _virtual_alloc_with_header(size, sizeof(MemoryArenaHeader), &pageSize);
		if (!addr)
			return 1;

		auto header = static_cast<MemoryArenaHeader*>(addr);
		header->capacity = size;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->commitSize = pageSize;
		header->pageSize = pageSize;
		header->next = nullptr;
		header->last = nullptr;
		header->alignSize = 1;
		header->flags = 0;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = 0;

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	i32 memory_arena_init(MemoryArena **arena, void *addr, usize size) {
		if (size < sizeof(MemoryArenaHeader))
			return 1;

		// Allocate the header
		auto header = static_cast<MemoryArenaHeader*>(addr);
		header->capacity = size;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->commitSize = size;
		header->pageSize = _get_page_size();
		header->next = nullptr;
		header->last = nullptr;
		header->alignSize = 1;
		header->flags = MemoryArenaHeader::SUB_ALLOCATED_BIT;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = 0;

#if HAS_ASAN
		__asan_poison_memory_region(add_ptr(addr, sizeof(MemoryArenaHeader)), size - sizeof(MemoryArenaHeader));
#endif

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void memory_arena_align_size(MemoryArena *arena, usize alignSize) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		header->alignSize = alignSize;
	}

	void memory_arena_destroy(MemoryArena *arena) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		if (header->flags & MemoryArenaHeader::SUB_ALLOCATED_BIT) {
			// The arena no longer manages the memory region referenced by addr
#if HAS_ASAN
			__asan_unpoison_memory_region(header, header->capacity);
#endif
		} else {
			auto capacity = header->capacity;
			decommit_region(header, header->commitSize);
			virtual_free(header, capacity);
		}
	}

	void* memory_arena_alloc(MemoryArena *arena, usize size, usize alignment) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		assert(alignment <= header->pageSize);
		assert(header->alignSize > 0);

		auto offset = align(header->usedMemory, alignment);
		auto alignedSize = align(size, header->alignSize);
		if (offset + alignedSize > header->capacity)
			return nullptr;

		if (_memory_arena_ensure_commit_size(header, offset + alignedSize) != 0)
			return nullptr;

		header->usedMemory = offset + alignedSize + ASAN_RED_ZONE_SIZE;
		header->allocationCount += 1;
		header->requestedMemory += size;

#if HAS_ASAN
		__asan_unpoison_memory_region(add_ptr(header, offset), size);
#endif

		return add_ptr(header, offset);
	}

	void memory_arena_free(MemoryArena *arena, void *addr, usize size) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		assert(header->alignSize > 0);

		auto alignedSize = align(size, header->alignSize);
		if (add_ptr(header, header->usedMemory - (alignedSize + ASAN_RED_ZONE_SIZE)) == addr)
			header->usedMemory -= alignedSize + ASAN_RED_ZONE_SIZE;

		header->requestedMemory -= size;
		header->allocationCount -= 1;

#if HAS_ASAN
		__asan_poison_memory_region(addr, size);
#endif
	}

	void* memory_arena_realloc(MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		if (!addr)
			return memory_arena_alloc(arena, nSize, alignment);

		auto header = bit_cast<MemoryArenaHeader*>(arena);

		{
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			assert(header->alignSize > 0);

			auto alignedSize = align(size, header->alignSize);
			auto offset = header->usedMemory - (alignedSize + ASAN_RED_ZONE_SIZE);
			if (add_ptr(header, offset) == addr) {
				if (offset + nSize > header->capacity)
					return nullptr;

				if (_memory_arena_ensure_commit_size(header, offset + nSize) != 0)
					return nullptr;

				header->usedMemory = offset + nSize + ASAN_RED_ZONE_SIZE;
				header->requestedMemory += nSize - size;

#if HAS_ASAN
				__asan_unpoison_memory_region(add_ptr(header, offset), nSize);
#endif

				return add_ptr(header, offset);
			}
		}

		auto nAddr = memory_arena_alloc(arena, nSize, alignment);
		if (!nAddr)
			return nullptr;
		memcpy(nAddr, addr, size);
		memory_arena_free(arena, addr, size);

		return nAddr;
	}

	void memory_arena_clear(MemoryArena *arena) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

#if HAS_ASAN
		__asan_poison_memory_region(
				add_ptr(header, sizeof(MemoryArenaHeader)),
				header->usedMemory - sizeof(MemoryArenaHeader));
#endif

		header->usedMemory = sizeof(MemoryArenaHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;
	}

	i32 memory_pool_init(MemoryArena **arena, usize size, usize objectSize) {
		usize pageSize;
		auto addr = _virtual_alloc_with_header(
				size, sizeof(MemoryArenaHeader) + sizeof(MemoryPoolHeader), &pageSize);

		if (!addr)
			return 1;

		// Allocate pool header
		auto header = static_cast<MemoryArenaHeader*>(addr);
		auto poolHeader = static_cast<MemoryPoolHeader*>(add_ptr(addr, sizeof(MemoryArenaHeader)));
		header->capacity = size;
		header->usedMemory = sizeof(MemoryArenaHeader) + sizeof(MemoryPoolHeader);
		header->commitSize = pageSize;
		header->pageSize = pageSize;
		header->alignSize = 1;
		header->flags = 0;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = 0;

		poolHeader->objectSize = align(objectSize, sizeof(void*));
		poolHeader->freeList = nullptr;

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void memory_pool_destroy(MemoryArena *arena) {
		memory_arena_destroy(arena);
	}

	void* memory_pool_alloc(MemoryArena *arena, usize size, usize alignment) {
		size = align(size, sizeof(void*));
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto poolHeader = static_cast<MemoryPoolHeader*>(add_ptr(header, sizeof(MemoryArenaHeader)));

		usize objectSize = poolHeader->objectSize;
		assert(size <= objectSize);

		{
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			if (poolHeader->freeList) {
				auto addr = poolHeader->freeList;
#if HAS_ASAN
				__asan_unpoison_memory_region(addr, size);
#endif
				poolHeader->freeList = *static_cast<void**>(addr);
				return addr;
			}
		}

		return memory_arena_alloc(arena, objectSize, alignment);
	}

	void memory_pool_free(MemoryArena *arena, void *addr, usize size) {
		size = align(size, sizeof(void*));
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto poolHeader = static_cast<MemoryPoolHeader*>(add_ptr(header, sizeof(MemoryArenaHeader)));

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		*static_cast<void**>(addr) = poolHeader->freeList;
		poolHeader->freeList = addr;

#if HAS_ASAN
		__asan_poison_memory_region(addr, size);
#endif
	}

	void* memory_pool_realloc(MemoryArena*, void *addr, [[maybe_unused]] usize size, [[maybe_unused]] usize nSize, usize) {
		assert(size == nSize);
		return addr;
	}

	void memory_pool_clear(MemoryArena *arena) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto poolHeader = static_cast<MemoryPoolHeader*>(add_ptr(header, sizeof(MemoryArenaHeader)));
		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		header->usedMemory = sizeof(MemoryArenaHeader) + sizeof(MemoryPoolHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;

		poolHeader->freeList = nullptr;

#if HAS_ASAN
		__asan_poison_memory_region(
				add_ptr(header, sizeof(MemoryArenaHeader) + sizeof(MemoryPoolHeader)),
				header->capacity - sizeof(MemoryArenaHeader) - sizeof(MemoryPoolHeader));
#endif
	}

	usize memory_pool_get_object_size(MemoryArena *arena) {
		auto poolHeader = static_cast<MemoryPoolHeader*>(add_ptr(arena, sizeof(MemoryArenaHeader)));
		return poolHeader->objectSize;
	}

	i32 memory_heap_init(MemoryArena **arena, usize size) {
		usize pageSize;
		auto addr = _virtual_alloc_with_header(
				size, sizeof(MemoryArenaHeader) + sizeof(MemoryHeapHeader), &pageSize);

		if (!addr)
			return 1;

		// Initialize headers
		auto header = static_cast<MemoryArenaHeader*>(addr);
		auto heapHeader = static_cast<MemoryHeapHeader*>(add_ptr(addr, sizeof(MemoryArenaHeader)));
		header->capacity = size;
		header->usedMemory = sizeof(MemoryArenaHeader) + sizeof(MemoryHeapHeader);
		header->commitSize = pageSize;
		header->pageSize = pageSize;
		header->next = nullptr;
		header->last = nullptr;
		header->alignSize = 1;
		header->flags = 0;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = _get_thread_id();

		heapHeader->minPoolObjectSize = 1 << 5;
		heapHeader->maxPoolObjectSize = 1 << (5 + sarray_count(heapHeader->poolFreeLists) - 1);
		heapHeader->heapSmallPageSize = 64 << 10;
		heapHeader->heapLargePageSize = 2 << 20;

		for (isize i = 0; i < sarray_count(heapHeader->poolFreeLists); ++i) {
			heapHeader->poolFreeLists[i] = nullptr;
		}

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void* memory_heap_alloc(MemoryArena *arena, usize size, usize alignment) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto heapHeader = static_cast<MemoryHeapHeader*>(add_ptr(arena, sizeof(MemoryArenaHeader)));

		assert(alignment <= header->pageSize);
		assert(sizeof(void*) <= heapHeader->minPoolObjectSize);

		usize objectSize;
		isize poolIdx = _memory_heap_pool_idx(&objectSize, heapHeader, size, alignment);
		assert(size <= objectSize);
		assert(heapHeader->minPoolObjectSize <= objectSize && objectSize <= heapHeader->maxPoolObjectSize);

		[[maybe_unused]] usize alignedSize = align(size, sizeof(void*));

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		if (poolIdx >= 0) {
			assert(poolIdx < sarray_count(heapHeader->poolFreeLists));

			void **freeList = heapHeader->poolFreeLists + poolIdx;

			if (!*freeList) {
				usize heapPageSize = heapHeader->heapSmallPageSize;
				if (size > heapHeader->heapSmallPageSize >> 1)
					heapPageSize = heapHeader->heapLargePageSize;
				assert(heapPageSize == align(heapPageSize, header->pageSize));
				void *addr = _memory_heap_alloc_pages(header, heapPageSize);
				if (!addr)
					return nullptr;

				memset(addr, 0, heapPageSize);

				// Build free list for page
				assert(objectSize < heapPageSize);
				isize slotCount = heapPageSize/objectSize;
				assert(slotCount > 1);
				for (isize i = 0; i < slotCount - 1; ++i) {
					void *slot = add_ptr(addr, i*objectSize);
					*static_cast<void**>(slot) = add_ptr(addr, (i + 1)*objectSize);
				}
				*static_cast<void**>(add_ptr(addr, (slotCount - 1)*objectSize)) = nullptr;
				*freeList = addr;
#if HAS_ASAN
				__asan_poison_memory_region(addr, heapPageSize);
#endif
			}

			void *addr = *freeList;
			assert(addr && addr > arena && addr < add_ptr(arena, header->capacity));
#if HAS_ASAN
			__asan_unpoison_memory_region(addr, alignedSize);
#endif
			*freeList = *static_cast<void**>(addr);
			++header->allocationCount;
			header->requestedMemory += size;

			return addr;
		}

		return nullptr;
	}

	void memory_heap_free(MemoryArena *arena, void *addr, usize size) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto heapHeader = static_cast<MemoryHeapHeader*>(add_ptr(arena, sizeof(MemoryArenaHeader)));

		usize objectSize;
		isize poolIdx = _memory_heap_pool_idx(&objectSize, heapHeader, size, 0);

		[[maybe_unused]] usize alignedSize = align(size, sizeof(void*));

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		assert(poolIdx >= 0);
		assert(addr > arena && addr < add_ptr(arena, header->capacity));

		if (poolIdx >= 0) {
			assert(poolIdx < sarray_count(heapHeader->poolFreeLists));
			void **freeList = heapHeader->poolFreeLists + poolIdx;

			*static_cast<void**>(addr) = *freeList;
			*freeList = addr;

			--header->allocationCount;
			header->requestedMemory -= size;

#if HAS_ASAN
			__asan_poison_memory_region(addr, alignedSize);
#endif
			return;
		}

		assert("unreachable" && false);
	}

	void* memory_heap_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		if (!addr)
			return memory_heap_alloc(arena, nSize, alignment);

		assert(nSize);

		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto heapHeader = static_cast<MemoryHeapHeader*>(add_ptr(arena, sizeof(MemoryArenaHeader)));

		usize objectSize;
		isize oldPoolIdx = _memory_heap_pool_idx(&objectSize, heapHeader, size, alignment);
		isize newPoolIdx = _memory_heap_pool_idx(&objectSize, heapHeader, nSize, alignment);

		[[maybe_unused]] usize nAlignedSize = align(nSize, sizeof(void*));
		assert(nSize <= objectSize);
		if (oldPoolIdx == newPoolIdx) {
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));
#if HAS_ASAN
			__asan_unpoison_memory_region(addr, nAlignedSize);
#endif
			header->requestedMemory += nSize - size;
			return addr;
		} else {
			auto nAddr = memory_heap_alloc(arena, nSize, alignment);
			if (!nAddr)
				return nullptr;

			auto copySize = size <= nSize ? size : nSize;
			memcpy(nAddr, addr, copySize);
			memory_heap_free(arena, addr, size);

			return nAddr;
		}
	}

	void memory_heap_clear(MemoryArena *arena) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto heapHeader = static_cast<MemoryHeapHeader*>(add_ptr(header, sizeof(MemoryArenaHeader)));

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		header->usedMemory = sizeof(MemoryArenaHeader) + sizeof(MemoryHeapHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;

		for (isize i = 0; i < sarray_count(heapHeader->poolFreeLists); ++i) {
			heapHeader->poolFreeLists[i] = nullptr;
		}

#if HAS_ASAN
		__asan_poison_memory_region(
				add_ptr(header, sizeof(MemoryArenaHeader) + sizeof(MemoryHeapHeader)),
				header->capacity - sizeof(MemoryArenaHeader) - sizeof(MemoryHeapHeader));
#endif
	}

	i32 mt_memory_arena_init(MemoryArena **arena, usize size) {
		usize pageSize;
		auto addr = _virtual_alloc_with_header(
				sizeof(MTMemoryArenaHeader), sizeof(MTMemoryArenaHeader), &pageSize);
		if (!addr)
			return 1;

		auto header = static_cast<MTMemoryArenaHeader*>(addr);

		header->threadArenaSize = size;
		header->totalAllocationCount = 0;
		header->totalRequestedMemory = 0;
		header->totalUsedMemory = 0;

		header->_lock = 0;
		header->first = nullptr;
		header->last = nullptr;

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void mt_memory_arena_destroy(MemoryArena *arena) {
		auto header = bit_cast<MTMemoryArenaHeader*>(arena);
		{
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			auto it = header->first;
			while (it) {
				auto localHeader = it;
				it = bit_cast<MemoryArenaHeader*>(it)->_nextArena;
				memory_arena_destroy(localHeader);
			}

		}

		decommit_region(header, sizeof(MTMemoryArenaHeader));
		virtual_free(header, sizeof(MTMemoryArenaHeader));
	}

	void* mt_memory_arena_alloc(MemoryArena *arena, usize size, usize alignment) {
		auto header = bit_cast<MTMemoryArenaHeader*>(arena);
		auto localArena = _require_thread_local_arena(header);
		if (!localArena)
			return nullptr;
		return memory_arena_alloc(localArena, size, alignment);
	}

	void mt_memory_arena_free(MemoryArena *arena, void *addr, usize size) {
		auto header = bit_cast<MTMemoryArenaHeader*>(arena);
		auto localArena = _require_thread_local_arena(header);
		if (!localArena)
			return;
		memory_arena_free(localArena, addr, size);
	}

	void* mt_memory_arena_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		auto header = bit_cast<MTMemoryArenaHeader*>(arena);
		auto localArena = _require_thread_local_arena(header);
		if (!localArena)
			return nullptr;
		return memory_arena_realloc(localArena, addr, size, nSize, alignment);
	}

	void mt_memory_arena_clear(MemoryArena *arena) {
		auto header = bit_cast<MTMemoryArenaHeader*>(arena);
		auto localArena = _require_thread_local_arena(header);
		if (!localArena)
			return;
		memory_arena_clear(localArena);
	}

	i32 sys_alloc_init(MemoryArena **arena) {
		usize pageSize;
		auto addr = _virtual_alloc_with_header(
				sizeof(MemoryArenaHeader), sizeof(MemoryArenaHeader), &pageSize);
		if (!addr)
			return 1;

		auto header = static_cast<MemoryArenaHeader*>(addr);
		header->capacity = 0;
		header->usedMemory = sizeof(MemoryArenaHeader);
		header->commitSize = pageSize;
		header->pageSize = pageSize;
		header->next = nullptr;
		header->last = nullptr;
		header->flags = 0;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = 0;

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void* sys_alloc(MemoryArena* arena, usize size, [[maybe_unused]] usize alignment) {
		if (!size)
			return nullptr;

		auto header = bit_cast<MemoryArenaHeader*>(arena);
		assert(alignment <= header->pageSize);
		auto alignedSize = align(size, header->pageSize);
		assert(alignedSize > 0);

		auto addr = virtual_alloc(alignedSize);
		if (!addr)
			return nullptr;
		if (commit_region(addr, alignedSize) != 0) {
			virtual_free(addr, alignedSize);
			return nullptr;
		}

#ifndef NDEBUG
		atomic_lock(&header->_lock);
		header->usedMemory += alignedSize;
		header->commitSize += alignedSize;
		header->requestedMemory += size;
		++header->allocationCount;
		atomic_unlock(&header->_lock);
#endif

#if HAS_ASAN
		__asan_unpoison_memory_region(addr, size);
#endif
		return addr;
	}

	void sys_free(MemoryArena* arena, void *addr, usize size) {
		if (!size)
			return;

		auto header = bit_cast<MemoryArenaHeader*>(arena);
		auto alignedSize = align(size, header->pageSize);

		decommit_region(addr, alignedSize);
		virtual_free(addr, alignedSize);

#ifndef NDEBUG
		atomic_lock(&header->_lock);
		assert(size <= header->requestedMemory);
		header->usedMemory -= alignedSize;
		header->commitSize -= alignedSize;
		header->requestedMemory -= size;
		--header->allocationCount;
		atomic_unlock(&header->_lock);
#endif
	}

	void* sys_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		if (!addr)
			return sys_alloc(arena, nSize, alignment);

		auto header = bit_cast<MemoryArenaHeader*>(arena);
		assert(alignment <= header->pageSize);
		assert(nSize >= size);
		assert(nSize);

		auto alignedSize = align(size, header->pageSize);
		auto nAlignedSize = align(nSize, header->pageSize);
		assert(nAlignedSize >= alignedSize);
		if (alignedSize >= nSize) {
#if HAS_ASAN
			__asan_unpoison_memory_region(addr, nSize);
#endif
			return addr;
		}

		if (virtual_try_grow(addr, alignedSize, nAlignedSize)) {
			auto nAddr = add_ptr(addr, alignedSize);
			isize dSize = nAlignedSize - alignedSize;
			assert(dSize > 0);
			assert(nSize >= size);
			if (commit_region(nAddr, dSize) == 0) {
#ifndef NDEBUG
				atomic_lock(&header->_lock);
				header->usedMemory += dSize;
				header->commitSize += dSize;
				header->requestedMemory += nSize - size;
				atomic_unlock(&header->_lock);
#endif

#if HAS_ASAN
				__asan_unpoison_memory_region(addr, nSize);
#endif
				return addr;
			}
			virtual_free(nAddr, dSize);
		}

		auto nAddr = sys_alloc(arena, nSize, alignment);
		if (!nAddr)
			return nullptr;
		memcpy(nAddr, addr, size);
		sys_free(arena, addr, size);
		return nAddr;
	}

	void sys_clear(MemoryArena*) {
	}

	Allocator make_arena_allocator(usize size) {
		Allocator allocator;
		if (memory_arena_init(&allocator.arena, size) != 0)
			return {};
		allocator.allocFn = memory_arena_alloc;
		allocator.freeFn = memory_arena_free;
		allocator.reallocFn = memory_arena_realloc;
		allocator.clearFn = memory_arena_clear;

		return allocator;
	}

	Allocator make_arena_allocator(void *addr, usize size) {
		Allocator allocator;
		if (memory_arena_init(&allocator.arena, addr, size) != 0)
			return {};
		allocator.allocFn = memory_arena_alloc;
		allocator.freeFn = memory_arena_free;
		allocator.reallocFn = memory_arena_realloc;
		allocator.clearFn = memory_arena_clear;

		return allocator;
	}

	Allocator make_pool_allocator(usize size, usize objectSize) {
		Allocator allocator;
		if (memory_pool_init(&allocator.arena, size, objectSize) != 0)
			return {};
		allocator.allocFn = memory_pool_alloc;
		allocator.freeFn = memory_pool_free;
		allocator.reallocFn = memory_pool_realloc;
		allocator.clearFn = memory_pool_clear;

		return allocator;
	}

	Allocator make_heap_allocator(usize size) {
		Allocator allocator;
		if (memory_heap_init(&allocator.arena, size) != 0)
			return {};
		allocator.allocFn = memory_heap_alloc;
		allocator.freeFn = memory_heap_free;
		allocator.reallocFn = memory_heap_realloc;
		allocator.clearFn = memory_heap_clear;

		return allocator;
	}

	Allocator make_mt_arena_allocator(usize size) {
		Allocator allocator;
		if (mt_memory_arena_init(&allocator.arena, size) != 0)
			return {};
		allocator.allocFn = mt_memory_arena_alloc;
		allocator.freeFn = mt_memory_arena_free;
		allocator.reallocFn = mt_memory_arena_realloc;
		allocator.clearFn = mt_memory_arena_clear;

		return allocator;
	}

	Allocator make_sys_allocator() {
		Allocator allocator;
		if (sys_alloc_init(&allocator.arena) != 0)
			return {};
		allocator.allocFn = sys_alloc;
		allocator.freeFn = sys_free;
		allocator.reallocFn = sys_realloc;
		allocator.clearFn = sys_clear;

		return allocator;
	}

	void* global_allocator_malloc(usize size) {
		static_assert(alignof(max_align_t) >= sizeof(size));
		auto result = globalAllocator->allocate(size + alignof(max_align_t), alignof(max_align_t));
		if (!result)
			return nullptr;
		memcpy(result, &size, sizeof(size));
		return add_ptr(result, alignof(max_align_t));
	}

	void* global_allocator_realloc(void *ptr, usize newSize) {
		usize size = 0;

		if (ptr) {
			ptr = sub_ptr(ptr, alignof(max_align_t));
			memcpy(&size, ptr, sizeof(size));
		}

		auto result = globalAllocator->realloc(
				ptr, size + alignof(max_align_t), newSize + alignof(max_align_t), alignof(max_align_t));
		if (!result)
			return nullptr;

		memcpy(result, &newSize, sizeof(newSize));
		return add_ptr(result, alignof(max_align_t));
	}

	void global_allocator_free(void *ptr) {
		if (!ptr)
			return;
		usize size;
		ptr = sub_ptr(ptr, alignof(max_align_t));
		memcpy(&size, ptr, sizeof(size));
		globalAllocator->deallocate(ptr, size);
	}

}

