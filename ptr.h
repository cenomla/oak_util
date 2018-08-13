#pragma once

#include <cstddef>
#include <cinttypes>

namespace oak {

	inline void* add_ptr(void *p, size_t x) {
		return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p) + x);
	}

	inline const void* add_ptr(const void *p, size_t x) {
		return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(p) + x);
	}

	inline void* sub_ptr(void* p, size_t x) {
		return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p) - x);
	}

	inline const void* sub_ptr(const void *p, size_t x) {
		return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(p) - x);
	}
	inline size_t align_size(size_t size, uint32_t alignment) {
		return (size + alignment-1) & (~(alignment-1));
	}

	inline uint32_t align_offset(const void *address, uint32_t alignment) {
		uint32_t adjustment = (alignment - (reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(alignment - 1)));
		return adjustment == alignment ? 0 : adjustment;
	}

	inline void* align_address(void *address, uint32_t alignment) {
		return add_ptr(address, align_offset(address, alignment));
	}

	inline uint32_t align_offset_with_header(const void *address, uint32_t alignment, uint32_t headerSize) {
		uint32_t adjustment = align_offset(address, alignment);

		uint32_t neededSpace = headerSize;

		if (adjustment < neededSpace) {
			neededSpace -= adjustment;

			adjustment += alignment * (neededSpace / alignment);

			if (neededSpace % alignment > 0) {
				adjustment += alignment;
			}
		}
		return adjustment;
	}

}
