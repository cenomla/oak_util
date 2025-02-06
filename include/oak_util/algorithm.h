#pragma once

#include <string.h>
#include <utility>

#include "containers.h"
#include "memory.h"

namespace oak {

	template<typename T>
	constexpr bool less(T const& lhs, T const& rhs) noexcept {
		return lhs < rhs;
	}

	constexpr bool is_lower(char c) noexcept {
		return c >= 'a' && c <= 'z';
	}

	constexpr bool is_upper(char c) noexcept {
		return c >= 'A' && c <= 'Z';
	}

	constexpr bool is_alpha(char c) noexcept {
		return is_lower(c) || is_upper(c);
	}

	constexpr bool is_space(char c) noexcept {
		return c == ' ' || c == '\n' || c == '\t' || c == '\v';
	}

	constexpr bool is_print(char c) noexcept {
		return c >= ' ' && c <= '~';
	}

	constexpr char to_lower(char c) noexcept {
		if (is_upper(c))
			return 'a' + c - 'A';
		return c;
	}

	constexpr char to_upper(char c) noexcept {
		if (is_lower(c))
			return 'A' + c - 'a';
		return c;
	}

	template<typename type>
	constexpr bool compare_case_insensitive(Slice<type> const lhs, Slice<type> const rhs) noexcept;

	template<>
	constexpr bool compare_case_insensitive(Slice<char const> const lhs, Slice<char const> const rhs) noexcept {
		if (lhs.count != rhs.count)
			return false;
		if (lhs.data == rhs.data)
			return true;

		for (i64 i = 0; i < lhs.count; ++i) {
			if (to_lower(lhs[i]) != to_lower(rhs[i]))
				return false;
		}

		return true;
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
		if (arrayCount < 2)
			return;

		auto temp = allocate<T>(allocator, arrayCount);
		SCOPE_EXIT(deallocate(allocator, temp, arrayCount));
		memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, std::forward<F>(functor));
	}

	template<typename T>
	constexpr void merge_sort(Allocator *allocator, T *array, i64 arrayCount) noexcept {
		if (arrayCount < 2)
			return;

		auto temp = allocate<T>(allocator, arrayCount);
		SCOPE_EXIT(deallocate(allocator, temp, arrayCount));
		memcpy(temp, array, arrayCount * sizeof(T));
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

	struct SortIndex {
		u32 rdx;
		i32 idx;
	};

	struct SortIndex64 {
		u64 rdx;
		i32 idx;
	};

	template<typename T, typename U, U T::* pMem = nullptr>
	constexpr void radix_sort(Allocator *allocator, T *array, i64 arrayCount, u32 r = 8) {
		auto t0 = array;
		auto t1 = allocate<T>(allocator, arrayCount);
		SCOPE_EXIT(deallocate(allocator, t1, arrayCount));

		u32 b = sizeof(U) * 8;

		// Num values per radix
		u32 rv = 1 << r;
		auto count = allocate<u32>(allocator, rv);
		SCOPE_EXIT(deallocate(allocator, count, rv));
		auto prefix = allocate<u32>(allocator, rv);
		SCOPE_EXIT(deallocate(allocator, prefix, rv));

		auto groups = b / r;
		u32 mask = rv - 1;

		for (u32 c = 0, shift = 0; c < groups; ++c, shift += r) {
			memset(count, 0, sizeof(u32) * rv);

			for (i64 i = 0; i < arrayCount; ++i) {
				++count[(t0[i].*pMem >> shift) & mask];
			}

			prefix[0] = 0;
			for (u32 i = 1; i < rv; ++i) {
				prefix[i] = prefix[i - 1] + count[i - 1];
			}

			for (i64 i = 0; i < arrayCount; ++i) {
				t1[prefix[(t0[i].*pMem >> shift) & mask]++] = t0[i];
			}

			swap(&t0, &t1);
		}

		assert(array == t0);
	}

	template<typename T>
	constexpr void radix_sort(Allocator *allocator, T *array, i64 arrayCount, u32 r = 8) {
		auto t0 = array;
		auto t1 = allocate<T>(allocator, arrayCount);
		SCOPE_EXIT(deallocate(allocator, t1, arrayCount));

		u32 b = sizeof(T) * 8;

		// Num values per radix
		u32 rv = 1 << r;
		auto count = allocate<u32>(allocator, rv);
		SCOPE_EXIT(deallocate(allocator, count, rv));
		auto prefix = allocate<u32>(allocator, rv);
		SCOPE_EXIT(deallocate(allocator, prefix, rv));

		auto groups = b / r;
		u32 mask = rv - 1;

		for (u32 c = 0, shift = 0; c < groups; ++c, shift += r) {
			memset(count, 0, sizeof(u32) * rv);

			for (i64 i = 0; i < arrayCount; ++i) {
				++count[(t0[i] >> shift) & mask];
			}

			prefix[0] = 0;
			for (u32 i = 1; i < rv; ++i) {
				prefix[i] = prefix[i - 1] + count[i - 1];
			}

			for (i64 i = 0; i < arrayCount; ++i) {
				t1[prefix[(t0[i] >> shift) & mask]++] = t0[i];
			}

			swap(&t0, &t1);
		}

		assert(array == t0);
	}

	template<typename T>
	constexpr void swap(T *a, T *b) noexcept {
		auto tmp = *a;
		*a = *b;
		*b = tmp;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* push(ArrayType *array, E const& value) {
		assert(array->count < array->capacity);
		array->data[array->count++] = value;
		return array->data + array->count - 1;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* try_push(ArrayType *array, E const& value) {
		if (array->count >= array->capacity)
			return nullptr;
		array->data[array->count++] = value;
		return array->data + array->count - 1;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* push_grow(ArrayType *array, Allocator *allocator, E const& value) {
		if (array->count == array->capacity) {
			array->reserve(allocator, array->capacity == 0 ? 4 : array->capacity * 2);
		}
		array->data[array->count++] = value;
		return array->data + array->count - 1;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* pop(ArrayType *array) {
		assert(array->count > 0);
		--array->count;
		return array->data + array->count;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* insert(ArrayType *array, i64 index, E const& value) noexcept {
		if (index == -1 || index == array->count) {
			return push(array, value);
		}
		assert(array->count < array->capacity);
		memmove(array->data + index + 1, array->data + index, (array->count++ - index) * sizeof(E));
		array->data[index] = value;

		return array->data + index;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr typename ArrayType::ElemType* insert_grow(ArrayType *array, Allocator *allocator, i64 index, E const& value) noexcept {
		if (index == -1 || index == array->count) {
			return push_grow(array, allocator, value);
		}
		if (array->count == array->capacity) {
			array->reserve(allocator, array->capacity == 0 ? 4 : array->capacity * 2);
		}
		memmove(array->data + index + 1, array->data + index, (array->count++ - index) * sizeof(E));
		array->data[index] = value;

		return array->data + index;
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr void remove(ArrayType *array, i64 index) noexcept {
		assert(array->count > 0);
		memmove(array->data + index, array->data + index + 1, (--array->count - index) * sizeof(E));
	}

	template<typename ArrayType>
	constexpr void swap_and_pop(ArrayType *array, i64 index) noexcept {
		array->data[index] = array->data[--array->count];
	}

	template<typename T>
	constexpr void fill(Slice<T> slice, T const& value) noexcept {
		for (i64 i = 0; i < slice.count; ++i) {
			slice[i] = value;
		}
	}

	template<typename T>
	constexpr void reverse(Slice<T> slice) noexcept {
		if (slice.count < 2) {
			return;
		}
		for (i64 i = 0; i < slice.count >> 1; ++i) {
			swap(slice.data + i, slice.data + slice.count - 1 - i);
		}
	}

	template<typename T, typename V>
	constexpr i64 find(Slice<T> slice, V const& value, i64 start = 0) noexcept {
		for (i64 i = start; i < slice.count; ++i) {
			if (slice[i] == value)
				return i;
		}
		return -1;
	}

	template<typename T, typename Functor>
	constexpr i64 find_pred(Slice<T> slice, Functor&& predFn, i64 start = 0) noexcept {
		for (i64 i = start; i < slice.count; ++i) {
			if (predFn(slice[i]))
				return i;
		}
		return -1;
	}

	template<typename T>
	constexpr i64 bfind(Slice<T> slice, T const& value, i64 first, i64 last) noexcept {
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

	template<typename T>
	constexpr Slice<T> sub_slice(Slice<T> slice, i64 start, i64 end = -1) noexcept {
		// Bounds checking
		if (end == -1 || end > slice.count)
			end = slice.count;

		return { slice.data + start, end - start };
	}

	template<typename T>
	constexpr i64 find_first_of(Slice<T> slice, Slice<T> delimeters, i64 start = 0) noexcept {
		for (i64 i = start; i < slice.count; i++) {
			for (i64 j = 0; j < delimeters.count; j++) {
				if (slice.data[i] == delimeters.data[j]) {
					return i;
				}
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_first_not_of(Slice<T> slice, Slice<T> delimeters, i64 start = 0) noexcept {
		bool found{};
		for (i64 i = start; i < slice.count; i++) {
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
	constexpr i64 find_last_of(Slice<T> slice, Slice<T> delimeters, i64 start = 0) noexcept {
		for (i64 i = slice.count; i > start; --i) {
			for (i64 j = 0; j < delimeters.count; ++j) {
				if (slice.data[i - 1] == delimeters.data[j]) {
					return i - 1;
				}
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_slice(Slice<T> slice, Slice<T> value, i64 start = 0) noexcept {
		if (slice.count < value.count)
			return -1;

		for (i64 i = start; i <= slice.count - value.count; i++) {
			if (value == Slice<T>{ slice.data + i, value.count }) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	constexpr i64 find_slice_case_insensitive(Slice<T> slice, Slice<T> value, i64 start = 0) noexcept;

	template<>
	constexpr i64 find_slice_case_insensitive(
			Slice<char const> slice, Slice<char const> value, i64 start) noexcept {
		if (slice.count < value.count)
			return -1;

		for (i64 i = start; i <= slice.count - value.count; i++) {
			if (compare_case_insensitive(value, Slice<char const>{ slice.data + i, value.count })) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	constexpr bool slice_starts_with(Slice<T> slice, Slice<T> value) noexcept {
		if (slice.count < value.count)
			return false;

		return sub_slice(slice, 0, value.count) == value;
	}

	template<typename T>
	constexpr i64 slice_count(Slice<T> slice, T value, i64 start = 0) noexcept {
		i64 count = 0;
		for (auto i = start; i < slice.count; ++i) {
			count += slice[i] == value;
		}
		return count;
	}

	template<typename T>
	constexpr Vector<Slice<T>> split_slice(
			Allocator *allocator, Slice<T> slice, Slice<T> delimeters) noexcept {
		Vector<Slice<T>> tokens;
		tokens.reserve(allocator, 32);

		i64 first = 0, last = 0;

		while (last != -1) {
			first = find_first_not_of(slice, delimeters, first);
			// Rest of string is entirely delimeters
			if (first == -1) {
				break;
			}
			last = find_first_of(slice, delimeters, first);
			push_grow(&tokens, allocator, sub_slice(slice, first, last));

			first = last + 1;
		}

		return tokens;
	}

	template<typename T, typename U>
	constexpr Slice<T> copy_slice(Allocator *allocator, Slice<U> slice, i64 minCapacity = 0) noexcept {
		Slice<T> nSlice;
		nSlice.count = slice.count;
		nSlice.data = allocate<T>(allocator, nSlice.count < minCapacity ? minCapacity : nSlice.count);
		if (!nSlice.data)
			return {};

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
		memcpy(dst.data, src.data, count * sizeof(T));
		dst.count = count;
		return count;
	}

	template<typename T, typename U>
	constexpr void concat_slice(Slice<T>& dst, Slice<U> srcA, Slice<U> srcB) noexcept {
		assert(srcA.count + srcB.count <= dst.count);
		for (i64 i = 0; i < dst.count; ++i) {
			auto const& src = i < srcA.count ? srcA[i] : srcB[i - srcA.count];
			dst[i] = src;
		}
	}

	OAK_UTIL_API bool is_c_str(String str) noexcept;
	OAK_UTIL_API char const* as_c_str(Allocator *allocator, String str) noexcept;
	OAK_UTIL_API String copy_str(Allocator *allocator, String str, bool isCStr = false) noexcept;

}

