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

	inline const uintptr_t ptr_diff(const void *p0, const void *p1) {
		auto min = reinterpret_cast<uintptr_t>(p0);
		auto max = reinterpret_cast<uintptr_t>(p1);
		if (min > max) {
			auto x = min;
			min = max;
			max = x;
		}
		return max - min;
	}

	inline size_t align_size(size_t size, size_t alignment) {
		return (size + alignment-1) & (~(alignment-1));
	}

	inline int64_t align_int(int64_t value, size_t alignment) {
		return (value + alignment - 1) & (~(alignment - 1));
	}

	inline size_t align_offset(const void *address, size_t alignment) {
		size_t adjustment = (alignment - (reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(alignment - 1)));
		return adjustment == alignment ? 0 : adjustment;
	}

	inline void* align_address(void *address, size_t alignment) {
		return add_ptr(address, align_offset(address, alignment));
	}

	inline size_t align_offset_with_header(const void *address, size_t alignment, size_t headerSize) {
		size_t adjustment = align_offset(address, alignment);

		size_t neededSpace = headerSize;

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
