#pragma once

#include "types.h"

namespace oak {

	inline void* add_ptr(void *p, i64 x) {
		return reinterpret_cast<void*>(reinterpret_cast<u64>(p) + x);
	}

	inline void const* add_ptr(void const *p, i64 x) {
		return reinterpret_cast<void const*>(reinterpret_cast<u64>(p) + x);
	}

	inline void* sub_ptr(void *p, i64 x) {
		return reinterpret_cast<void*>(reinterpret_cast<u64>(p) - x);
	}

	inline void const* sub_ptr(void const *p, i64 x) {
		return reinterpret_cast<void const*>(reinterpret_cast<u64>(p) - x);
	}

	inline u64 ptr_diff(void const *p0, void const *p1) {
		auto min = reinterpret_cast<u64>(p0);
		auto max = reinterpret_cast<u64>(p1);
		if (min > max) {
			auto x = min;
			min = max;
			max = x;
		}
		return max - min;
	}

	constexpr u64 align_size(u64 size, u64 alignment) {
		return (size + alignment - 1) & (~(alignment-1));
	}

	constexpr i64 align_int(i64 value, u64 alignment) {
		return (value + alignment - 1) & (~(alignment - 1));
	}

	inline u64 align_offset(void const *address, u64 alignment) {
		u64 adjustment = (alignment - (reinterpret_cast<u64>(address) & static_cast<u64>(alignment - 1)));
		return adjustment == alignment ? 0 : adjustment;
	}

	inline void* align_address(void *address, u64 alignment) {
		return add_ptr(address, align_offset(address, alignment));
	}

	inline u64 align_offset_with_header(void const *address, u64 alignment, u64 headerSize) {
		u64 adjustment = align_offset(address, alignment);

		u64 neededSpace = headerSize;

		if (adjustment < neededSpace) {
			neededSpace -= adjustment;

			adjustment += alignment * (neededSpace / alignment);

			if (neededSpace % alignment > 0) {
				adjustment += alignment;
			}
		}
		return adjustment;
	}

	template<typename... types>
	constexpr u64 max_align() noexcept {
		constexpr u64 aligns[] = { alignof(types)... };

		// Calculate the max alignment of the types
		u64 align = 0;
		for (auto elem : aligns) {
			if (elem > align) {
				align = elem;
			}
		}

		return align;
	}

	template<int index, typename... types>
	constexpr i64 soa_offset(i64 const count) noexcept {
		constexpr u64 sizes[] = { sizeof(types)... };
		constexpr u64 aligns[] = { alignof(types)... };

		i64 offset = 0;
		for (i32 i = 0; i < index; ++i) {
			offset = align_int(offset, aligns[i]);
			offset += count * sizes[i];
		}

		return offset;
	}

}

