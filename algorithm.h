#pragma once

namespace oak {

	struct IAllocator;

	template<typename T>
	void merge_sort(Array<T>& array, IAllocator *allocator) {
		static auto top_down_merge = [](Array<T>& array, size_t begin,
				size_t middle, size_t end, Array<T>& buffer) {
			auto i = begin, j = middle;
			for (auto k = begin; k < end; k++) {
				if (i < middle && (j >= end || array[i] <= array[j])) {
					buffer[k] = array[i];
					i ++;
				} else {
					buffer[k] = array[j];
					j ++;
				}
			}
		};
		
		static void(*top_down_split_merge)(Array<T>&, size_t, size_t, Array<T>&) =
			[](Array<T>& buffer, 
				size_t begin, size_t end, Array<T>& array) {
			if (end - begin < 2) {
				return;
			}
			auto middle = (end + begin) / 2;
			top_down_split_merge(array, begin, middle, buffer);
			top_down_split_merge(array, middle, end, buffer);
			top_down_merge(buffer, begin, middle, end, array);
		};
		auto temp = array.clone(allocator);
		top_down_split_merge(temp, 0, array.size, array);
	}

}
