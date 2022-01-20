#pragma once

#include <utility>
#include <cstring>

#include "memory.h"

namespace oak {

	template<typename T>
	constexpr bool less(T const& lhs, T const& rhs) noexcept {
		return lhs < rhs;
	}

	namespace detail {
		template<typename T, typename F>
		constexpr void ms_impl_merge(T *array, T *buffer, i64 begin, i64 middle, i64 end, F&& functor) noexcept {
			auto i = begin, j = middle;
			for (auto k = begin; k < end; k++) {
				if (i < middle && (j >= end || !functor(array[j], array[i]))) {
					buffer[k] = array[i];
					i ++;
				} else {
					buffer[k] = array[j];
					j ++;
				}
			}
		}

		template<typename T, typename F>
		constexpr void ms_impl_split(T *array, T *buffer, i64 begin, i64 end, F&& functor) noexcept {
			if (end - begin < 2) {
				return;
			}
			auto middle = (end + begin) / 2;
			ms_impl_split(buffer, array, begin, middle, std::forward<F>(functor));
			ms_impl_split(buffer, array, middle, end, std::forward<F>(functor));
			ms_impl_merge(buffer, array, begin, middle, end, std::forward<F>(functor));
		}

		template<typename T>
		constexpr void qs_impl_swap(T& a, T& b) noexcept {
			auto tmp = a;
			a = b;
			b = tmp;
		}

		template<typename T, typename F>
		constexpr void qs_impl(T *array, i64 start, i64 end, F&& functor) noexcept {
			if (start < end) {
				auto l = start + 1, r = end;
				auto p = array[start];
				while (l < r) {
					if (functor(array[l], p)) {
						++l;
					} else if (!functor(array[r], p)) {
						--r;
					} else {
						qs_impl_swap(array[l], array[r]);
					}
				}
				if (functor(array[l], p)) {
					qs_impl_swap(array[l], array[start]);
					--l;
				} else {
					--l;
					qs_impl_swap(array[l], array[start]);
				}
				qs_impl(array, start, l, std::forward<F>(functor));
				qs_impl(array, r, end, std::forward<F>(functor));
			}
		}
	}

	template<typename T, typename F>
	constexpr void merge_sort(Allocator *allocator, T *array, i64 arrayCount, F&& functor) noexcept {
		if (arrayCount < 2) { return; }
		auto temp = allocate<T>(allocator, arrayCount);
		std::memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, std::forward<F>(functor));
	}

	template<typename T>
	constexpr void merge_sort(Allocator *allocator, T *array, i64 arrayCount) noexcept {
		if (arrayCount < 2) { return; }
		auto temp = allocate<T>(allocator, arrayCount);
		std::memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, less<T>);
	}

	template<typename T, typename F>
	constexpr void quick_sort(MemoryArena*, T *array, i64 arrayCount, F&& functor) noexcept {
		detail::qs_impl(array, 0, arrayCount - 1, std::forward<F>(functor));
	}

	template<typename T>
	constexpr void quick_sort(MemoryArena*, T *array, i64 arrayCount) noexcept {
		detail::qs_impl(array, 0, arrayCount - 1, less<T>);
	}

	template<typename T>
	constexpr void swap(T *a, T *b) noexcept {
		auto tmp = *a;
		*a = *b;
		*b = tmp;
	}

	template<typename T>
	constexpr void swap_and_pop(Slice<T>& slice, i64 const index) noexcept {
		slice[index] = slice[--slice.count];
	}

	template<typename T>
	constexpr void insert(Slice<T>& slice, i64 const index, T const& value) noexcept {
		std::memmove(slice.data + index + 1, slice.data + index, (slice.count++ - index) * sizeof(T));
		slice[index] = value;
	}

	template<typename T>
	constexpr void remove(Slice<T>& slice, i64 const index) noexcept {
		std::memmove(slice.data + index, slice.data + index + 1, (--slice.count - index) * sizeof(T));
	}

	template<typename T, typename V>
	constexpr i64 bfind(Slice<T> const& slice, V const& value, i64 const first, i64 const last) noexcept {
		assert(first >= 0 && first < slice.count);
		assert(last >= 0 && last < slice.count);
		assert(first <= last);
		auto partIdx = first + (last + 1 - first) / 2;
		if (slice[partIdx] < value && (last - partIdx) > 0) {
			return bfind(slice, value, partIdx + 1, last);
		} else if (slice[partIdx] > value && (partIdx - first) > 0) {
			return bfind(slice, value, first, partIdx - 1);
		} else if (slice[partIdx] == value) {
			return partIdx;
		} else {
			return -1;
		}
	}

	template<typename T, typename V>
	constexpr i64 find(Slice<T> const slice, V const& value, i64 const start = 0) noexcept {
		for (i64 i = start; i < slice.count; ++i) {
			if (slice[i] == value) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	constexpr Slice<T> sub_slice(Slice<T> const slice, i64 start, i64 end = -1) noexcept {
		// Bounds checking
		if (end == -1) { end = slice.count; }
		return { slice.data + start, end - start };
	}

	template<typename T>
	constexpr i64 find_first_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) noexcept {
		for (auto i = start; i < slice.count; i++) {
			for (i64 j = 0; j < delimeters.count; j++) {
				if (slice.data[i] == delimeters.data[j]) {
					return i;
				}
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_first_not_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) noexcept {
		bool found{};
		for (auto i = start; i < slice.count; i++) {
			found = false;
			for (i64 j = 0; j < delimeters.count; j++) {
				if (slice.data[i] == delimeters.data[j]) {
					found = true;
					break;
				}
			}
			if (!found) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_last_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) noexcept {
		for (auto i = slice.count; i > start; --i) {
			for (i64 j = 0; j < delimeters.count; ++j) {
				if (slice.data[i - 1] == delimeters.data[j]) {
					return i - 1;
				}
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_slice(Slice<T> const slice, Slice<T> const value, i64 start = 0) noexcept {
		if (slice.count < value.count) { return -1; }
		for (auto i = start; i <= slice.count - value.count; i++) {
			if (value == Slice<T>{ slice.data + i, value.count }) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	constexpr Slice<Slice<T>> split_slice(Slice<T> const slice, Slice<T> const delimeters) noexcept {
		i64 tokenCapacity = 64;

		Slice<Slice<T>> tokens;
		tokens.data = allocate<Slice<T>>(temporaryAllocator, tokenCapacity);

		i64 first = 0, last = 0;

		while (last != -1) {
			first = find_first_not_of(slice, delimeters, first);
			if (first == -1) { break; } // Rest of string is entirely delimeters
			last = find_first_of(slice, delimeters, first);
			if (tokens.count == tokenCapacity) {
				tokenCapacity *= 2;
				auto ndata = allocate<Slice<T>>(temporaryAllocator, tokenCapacity);
				std::memcpy(ndata, tokens.data, tokens.count);
				tokens.data = ndata;
			}
			tokens[tokens.count++] = sub_slice(slice, first, last);

			first = last + 1;
		}

		return tokens;
	}

	template<typename T>
	constexpr void reverse(Slice<T>& slice) noexcept {
		if (slice.count < 2) { return; }
		for (i64 i = 0; i < slice.count >> 1; ++i) {
			auto tmp = slice[i];
			slice[i] = slice[slice.count - 1 - i];
			slice[slice.count - 1 - i] = tmp;
		}
	}

	template<typename T, typename U>
	constexpr Slice<T> copy_slice(Allocator *allocator, Slice<U> const slice, i64 minCapacity = 0) noexcept {
		Slice<T> nSlice;
		nSlice.count = slice.count;
		nSlice.data = allocate<T>(allocator, minCapacity > nSlice.count ? minCapacity : nSlice.count);
		if (!nSlice.data) {
			return {};
		}
		for (i64 i = 0; i < nSlice.count; ++i) {
			nSlice[i] = slice[i];
		}
		return nSlice;
	}

	template<typename T, typename U, usize N, usize M>
	constexpr i64 copy_array(Array<T, N>& dst, Array<U, M> const& src) {
		static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<U>>);
		auto count = src.count;
		if (count > dst.capacity) {
			count = dst.capacity;
		}
		std::memcpy(dst.data, src.data, count * sizeof(T));
		dst.count = count;
		return count;
	}

	template<typename T, typename U>
	constexpr void concat_slice(Slice<T>& dst, Slice<U> const srcA, Slice<U> const srcB) noexcept {
		assert(srcA.count + srcB.count <= dst.count);
		for (i64 i = 0; i < dst.count; ++i) {
			auto const& src = i < srcA.count ? srcA[i] : srcB[i - srcA.count];
			dst[i] = src;
		}
	}

	constexpr u64 hash_combine(u64 const a, u64 const b) noexcept {
		// Combine the two hash values using a bunch of random large primes
		return 262147 + a * 131101 + b * 65599;
	}

	OAK_UTIL_API bool is_c_str(String const str) noexcept;
	OAK_UTIL_API char const* as_c_str(String const str) noexcept;
	OAK_UTIL_API String copy_str(Allocator *allocator, String const str) noexcept;

}

