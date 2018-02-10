#pragma once

#include <cstddef>

namespace oak {

	template<typename T>
	struct ArrayView {
		T *data = nullptr;
		size_t size = 0;

		T& operator[](size_t idx) { return data[idx]; }
		const T& operator[](size_t idx) const { return data[idx]; }

		inline T* begin() { return data; }
		inline T* end() { return data + size; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + size; }
	};

}
