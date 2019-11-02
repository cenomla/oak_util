#pragma once

#include "types.h"

namespace oak {

	inline void* add_ptr(void *p, i64 x) noexcept {
		return reinterpret_cast<void*>(reinterpret_cast<u64>(p) + x);
	}

	inline void const* add_ptr(void const *p, i64 x) noexcept {
		return reinterpret_cast<void const*>(reinterpret_cast<u64>(p) + x);
	}

	inline void* sub_ptr(void *p, i64 x) noexcept {
		return reinterpret_cast<void*>(reinterpret_cast<u64>(p) - x);
	}

	inline void const* sub_ptr(void const *p, i64 x) noexcept {
		return reinterpret_cast<void const*>(reinterpret_cast<u64>(p) - x);
	}

	inline u64 ptr_diff(void const *p0, void const *p1) noexcept {
		auto min = reinterpret_cast<u64>(p0);
		auto max = reinterpret_cast<u64>(p1);
		if (min > max) {
			auto x = min;
			min = max;
			max = x;
		}
		return max - min;
	}

	constexpr u64 align(u64 const value, u64 const alignment) noexcept {
		return (value + alignment - 1) & (~(alignment-1));
	}

	constexpr i64 align(i64 const value, u64 const alignment) noexcept {
		return (value + alignment - 1) & (~(alignment - 1));
	}

	constexpr u64 align_offset(u64 const value, u64 const alignment) noexcept {
		auto adjustment = (alignment - (value & (alignment - 1)));
		return adjustment == alignment ? 0 : adjustment;
	}

	inline u64 align_offset(void const *const address, u64 const alignment) noexcept {
		return align_offset(reinterpret_cast<u64>(address), alignment);
	}

	inline void* align(void *const address, u64 const alignment) noexcept {
		return add_ptr(address, align_offset(address, alignment));
	}

	inline void const* align(void const *const address, u64 const alignment) noexcept {
		return add_ptr(address, align_offset(address, alignment));
	}

	constexpr u64 align_offset_with_header(u64 const value, u64 const alignment, u64 const headerSize) noexcept {
		auto adjustment = align_offset(value, alignment);

		if (adjustment < headerSize) {
			adjustment = align_offset(value + headerSize, alignment);
		}

		return adjustment;
	}

	inline u64 align_offset_with_header(void const *address, u64 const alignment, u64 const headerSize) noexcept {
		return align_offset_with_header(reinterpret_cast<u64>(address), alignment, headerSize);
	}

	template<typename... Types>
	constexpr u64 max_size() noexcept {
		constexpr u64 sizes[] = { sizeof(Types)... };

		// Calculate the max alignment of the types
		u64 size = 0;
		for (auto elem : sizes) {
			if (elem > size) {
				size = elem;
			}
		}

		return size;
	}

	template<typename... Types>
	constexpr u64 max_align() noexcept {
		constexpr u64 aligns[] = { alignof(Types)... };

		// Calculate the max alignment of the types
		u64 align = 0;
		for (auto elem : aligns) {
			if (elem > align) {
				align = elem;
			}
		}

		return align;
	}

	template<int Index, typename... Types>
	constexpr i64 soa_offset(i64 const count) noexcept {
		constexpr u64 sizes[] = { sizeof(Types)... };
		constexpr u64 aligns[] = { alignof(Types)... };

		i64 offset = 0;
		i32 i = 0;
		for (; i < Index; ++i) {
			offset = align(offset, aligns[i]);
			offset += count * sizes[i];
		}

		if (i < static_cast<i64>(sizeof...(Types))) {
			offset = align(offset, aligns[i]);
		}

		return offset;
	}

}

