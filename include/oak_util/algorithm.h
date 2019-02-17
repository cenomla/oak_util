#pragma once

#include <utility>

#include "memory.h"

namespace oak {

	template<typename T>
	inline bool less(T const& lhs, T const& rhs) {
		return lhs < rhs;
	}

	namespace detail {
		template<typename T, typename F>
		void ms_impl_merge(T *array, T *buffer, int64_t begin, int64_t middle, int64_t end, F&& functor) {
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
		void ms_impl_split(T *array, T *buffer, int64_t begin, int64_t end, F&& functor) {
			if (end - begin < 2) {
				return;
			}
			auto middle = (end + begin) / 2;
			ms_impl_split(buffer, array, begin, middle, std::forward<F>(functor));
			ms_impl_split(buffer, array, middle, end, std::forward<F>(functor));
			ms_impl_merge(buffer, array, begin, middle, end, std::forward<F>(functor));
		}

		template<typename T>
		void qs_impl_swap(T& a, T& b) {
			auto tmp = a;
			a = b;
			b = tmp;
		}

		template<typename T, typename F>
		void qs_impl(T *array, int64_t start, int64_t end, F&& functor) {
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
	void merge_sort(MemoryArena *arena, T *array, int64_t arrayCount, F&& functor) {
		if (arrayCount < 2) { return; }
		auto temp = allocate<T>(arena, arrayCount);
		std::memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, std::forward<F>(functor));
	}

	template<typename T, typename F>
	void merge_sort(MemoryArena *arena, T *array, int64_t arrayCount) {
		auto temp = allocate<T>(arena, arrayCount);
		std::memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, less<T>);
	}

	template<typename T, typename F>
	void quick_sort(MemoryArena*, T *array, int64_t arrayCount, F&& functor) {
		detail::qs_impl(array, 0, arrayCount - 1, std::forward<F>(functor));
	}

	template<typename T, typename F>
	void quick_sort(MemoryArena*, T *array, int64_t arrayCount) {
		detail::qs_impl(array, 0, arrayCount - 1, less<T>);
	}

	template<typename T>
	constexpr i64 find(Slice<T> const& slice, T const& value, i64 const start = 0) noexcept {
		for (i64 i = start; i < slice.count; ++i) {
			if (slice[i] == value) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	Slice<T> sub_slice(Slice<T> const slice, i64 start, i64 end = -1) {
		// Bounds checking
		if (end == -1) { end = slice.count; }
		return { slice.data + start, end - start };
	}

	template<typename T>
	i64 find_first_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) {
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
	i64 find_first_not_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) {
		bool found;
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
	i64 find_last_of(Slice<T> const slice, Slice<T> const delimeters, i64 start = 0) {
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
	i64 find_slice(Slice<T> const slice, Slice<T> const value, i64 start = 0) {
		if (slice.count < value.count) { return -1; }
		for (auto i = start; i <= slice.count - value.count; i++) {
			if (value == Slice<T>{ slice.data + i, value.count }) {
				return i;
			}
		}
		return -1;
	}

	template<typename T>
	Slice<Slice<T>> splitstr(Slice<T> const slice, Slice<T> const delimeters) {
		i64 tokenCapacity = 64;

		Slice<Slice<T>> tokens;
		tokens.data = allocate<Slice<T>>(temporaryMemory, tokenCapacity);

		i64 first = 0, last = 0;

		while (last != -1) {
			first = find_first_not_of(slice, delimeters, first);
			if (first == -1) { break; } // Rest of string is entirely delimeters
			last = find_first_of(slice, delimeters, first);
			if (tokens.count == tokenCapacity) {
				tokenCapacity *= 2;
				auto ndata = allocate<Slice<T>>(temporaryMemory, tokenCapacity);
				memcpy(ndata, tokens.data, tokens.count);
				tokens.data = ndata;
			}
			tokens[tokens.count++] = sub_slice(slice, first, last);

			first = last + 1;
		}

		return tokens;
	}

	template<typename T>
	constexpr void reverse(Slice<T>& slice) {
		if (slice.count < 2) { return; }
		for (i64 i = 0; i < slice.count >> 1; ++i) {
			auto tmp = slice[i];
			slice[i] = slice[slice.count - 1 - i];
			slice[slice.count - 1 - i] = tmp;
		}
	}

	bool is_c_str(String const str);
	const char* as_c_str(String const str);
	String copy_str(Allocator *allocator, String const str);

}

