#pragma once

#include <cstddef>

namespace oak {

	template<typename T>
	struct Slice {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef T value_type; 

		Slice() = default;
		Slice(T *_data, size_t _size) : data{ _data }, size{ _size } {}
		template<size_t C>
		Slice(T (&array)[C]) : data{ &array[0] }, size{ C } {}

		size_t find(const T& v) const {
			for (size_t i = 0; i < size; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return npos;
		}

		T& operator[](size_t idx) { return data[idx]; }
		const T& operator[](size_t idx) const { return data[idx]; }

		inline T* begin() { return data; }
		inline T* end() { return data + size; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + size; }

		T *data = nullptr;
		size_t size = 0;
	};

}
