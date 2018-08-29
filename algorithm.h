#pragma once

#include "memory.h"

namespace oak {

	template<typename T>
	inline bool less(const T& lhs, const T& rhs) {
		return lhs < rhs;
	}

	template<typename T, typename F>
	void merge_sort(MemoryArena *arena, T *array, int64_t arrayCount, F&& functor) {
		static auto top_down_merge = [&functor](T *array, T *buffer, int64_t begin, int64_t middle, int64_t end) {
			auto i = begin, j = middle;
			for (auto k = begin; k < end; k++) {
				if (i < middle && (j >= end || functor(array[i], array[j]))) {
					buffer[k] = array[i];
					i ++;
				} else {
					buffer[k] = array[j];
					j ++;
				}
			}
		};

		static void(*top_down_split_merge)(T*, T*, int64_t, int64_t) =
			[](T *array, T *buffer, int64_t begin, int64_t end) {
			if (end - begin < 2) {
				return;
			}
			auto middle = (end + begin) / 2;
			top_down_split_merge(buffer, array, begin, middle);
			top_down_split_merge(buffer, array, middle, end);
			top_down_merge(buffer, array, begin, middle, end);
		};
		auto temp = allocate_structs<T>(arena, arrayCount);
		for (int64_t i = 0; i < arrayCount; i++) {
			temp[i] = array[i];
		}
		top_down_split_merge(array, temp, 0, arrayCount);
	}

}

