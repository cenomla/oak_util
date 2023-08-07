#define OAK_UTIL_EXPORT_SYMBOLS
#include <oak_util/memory.h>

#ifdef _WIN32
#else
#include <sanitizer/asan_interface.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif // _WIN32

#include <stdlib.h>

#include <oak_util/atomic.h>
#include <oak_util/types.h>
#include <oak_util/ptr.h>
#include <oak_util/bit.h>

namespace oak {

namespace {

	static thread_local MemoryArena *_threadLocalArena = nullptr;

	usize _get_page_size() {
#ifdef _WIN32
#else
		auto scResult = sysconf(_SC_PAGE_SIZE);
		if (scResult == -1)
			return 1;
		return static_cast<usize>(scResult);
#endif
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

		if (memory_arena_init(&_threadLocalArena, header->threadArenaSize) != 0)
			return nullptr;

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		if (header->last)
			bit_cast<MemoryArenaHeader*>(header->last)->_nextArena = _threadLocalArena;
		header->last = _threadLocalArena;
		if (!header->first)
			header->first = header->last;

		return _threadLocalArena;
	}

}

	void* virtual_alloc(usize size) {
#ifdef _WIN32
#else
		auto result = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (result == MAP_FAILED)
			return nullptr;

		return result;
#endif // _WIN32
	}

	bool virtual_try_grow(void *addr, usize size, usize nSize) {
#ifdef _WIN32
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
#else
		auto result = munmap(addr, size);
		assert(result == 0);
#endif // _WIN32
	}

	i32 commit_region(void *addr, usize size) {
#ifdef _WIN32
#else
		if (mprotect(addr, size, PROT_READ|PROT_WRITE) == -1)
			return 1;
		ASAN_POISON_MEMORY_REGION(addr, size);
		return 0;
#endif
	}

	i32 decommit_region(void *addr, usize size) {
#ifdef _WIN32
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
		auto pageSize = _get_page_size();
		size = align(size, pageSize);
		auto addr = virtual_alloc(size);
		if (!addr)
			return 1;

		// Allocate the header
		if (commit_region(addr, pageSize) != 0) {
			virtual_free(addr, size);
			return 1;
		}
		ASAN_UNPOISON_MEMORY_REGION(addr, sizeof(MemoryArenaHeader));

		auto header = static_cast<MemoryArenaHeader*>(addr);
		header->capacity = size;
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
		header->flags = 0;

		header->allocationCount = 0;
		header->requestedMemory = 0;

		header->_lock = 0;
		header->_nextArena = nullptr;
		header->_threadId = 0;

		ASAN_POISON_MEMORY_REGION(add_ptr(addr, sizeof(MemoryArenaHeader)), size - sizeof(MemoryArenaHeader));

		*arena = static_cast<MemoryArena*>(addr);

		return 0;
	}

	void memory_arena_destroy(MemoryArena *arena) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		if (header->pageSize) {
			auto capacity = header->capacity;
			decommit_region(header, header->commitSize);
			virtual_free(header, capacity);
		} else {
			// The arena no longer manages the memory region referenced by addr
			ASAN_UNPOISON_MEMORY_REGION(header, header->capacity);
		}
	}

	void* memory_arena_alloc(MemoryArena *arena, usize size, usize alignment) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);
		assert(alignment <= header->pageSize);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		auto offset = align(header->usedMemory, alignment);
		if (offset + size > header->capacity)
			return nullptr;

		if (_memory_arena_ensure_commit_size(header, offset + size) != 0)
			return nullptr;

		header->usedMemory = offset + size;
		header->allocationCount += 1;
		header->requestedMemory += size;

		ASAN_UNPOISON_MEMORY_REGION(add_ptr(header, offset), size);

		return add_ptr(header, offset);
	}

	void memory_arena_free(MemoryArena *arena, void *addr, usize size) {
		auto header = bit_cast<MemoryArenaHeader*>(arena);

		atomic_lock(&header->_lock);
		SCOPE_EXIT(atomic_unlock(&header->_lock));

		if (add_ptr(header, header->usedMemory - size) == addr)
			header->usedMemory -= size;

		header->requestedMemory -= size;
		header->allocationCount -= 1;
		ASAN_POISON_MEMORY_REGION(addr, size);
	}

	void* memory_arena_realloc(MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		if (!addr)
			return memory_arena_alloc(arena, nSize, alignment);

		auto header = bit_cast<MemoryArenaHeader*>(addr);

		{
			atomic_lock(&header->_lock);
			SCOPE_EXIT(atomic_unlock(&header->_lock));

			auto offset = header->usedMemory - size;
			if (add_ptr(header, offset) == addr) {
				if (offset + nSize > header->capacity)
					return nullptr;

				if (_memory_arena_ensure_commit_size(header, offset + nSize) != 0)
					return nullptr;

				header->usedMemory = offset + nSize;
				header->requestedMemory += nSize - size;

				ASAN_UNPOISON_MEMORY_REGION(add_ptr(header, offset), nSize);

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

		header->usedMemory = sizeof(MemoryArenaHeader);
		header->allocationCount = 0;
		header->requestedMemory = 0;
		ASAN_POISON_MEMORY_REGION(
				add_ptr(header, sizeof(MemoryArenaHeader)),
				header->capacity - sizeof(MemoryArenaHeader));
	}

	i32 mt_memory_arena_init(MemoryArena **arena, usize size) {
		auto pageSize = _get_page_size();
		assert(sizeof(MTMemoryArenaHeader) <= pageSize);

		auto addr = virtual_alloc(pageSize);
		if (!addr)
			return 1;

		// Allocate the header
		if (commit_region(addr, pageSize) != 0) {
			virtual_free(addr, pageSize);
			return 1;
		}
		ASAN_UNPOISON_MEMORY_REGION(addr, sizeof(MTMemoryArenaHeader));

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

	void* sys_alloc(MemoryArena*, usize size, usize alignment) {
		if (!size)
			return nullptr;

		auto pageSize = _get_page_size();
		assert(alignment <= pageSize);
		size = align(size, pageSize);

		auto addr = virtual_alloc(size);
		if (!addr)
			return nullptr;
		if (commit_region(addr, size) != 0) {
			virtual_free(addr, size);
			return nullptr;
		}

		ASAN_UNPOISON_MEMORY_REGION(addr, size);
		return addr;
	}

	void sys_free(MemoryArena*, void *addr, usize size) {
		if (!size)
			return;
		size = align(size, _get_page_size());

		decommit_region(addr, size);
		virtual_free(addr, size);
	}

	void* sys_realloc(
			MemoryArena *arena, void *addr, usize size, usize nSize, usize alignment) {
		if (!addr)
			return sys_alloc(arena, nSize, alignment);

		auto pageSize = _get_page_size();
		assert(alignment <= pageSize);

		size = align(size, pageSize);
		if (size >= nSize)
			return addr;
		nSize = align(nSize, pageSize);


		if (virtual_try_grow(addr, size, nSize)) {
			auto nAddr = add_ptr(addr, size);
			auto dSize = nSize - size;
			if (commit_region(nAddr, dSize) == 0) {
				ASAN_UNPOISON_MEMORY_REGION(nAddr, dSize);
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
		allocator.arena = nullptr;
		allocator.allocFn = sys_alloc;
		allocator.freeFn = sys_free;
		allocator.reallocFn = sys_realloc;
		allocator.clearFn = sys_clear;

		return allocator;
	}

	Allocator* globalAllocator;
	Allocator* temporaryAllocator;

}

