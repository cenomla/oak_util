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
		auto temp = allocate_structs<T>(arena, arrayCount);
		std::memcpy(temp, array, arrayCount * sizeof(T));
		detail::ms_impl_split(array, temp, 0, arrayCount, std::forward<F>(functor));
	}

	template<typename T, typename F>
	void merge_sort(MemoryArena *arena, T *array, int64_t arrayCount) {
		auto temp = allocate_structs<T>(arena, arrayCount);
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


}

