#pragma once

#include <cstddef>
#include <type_traits>

namespace oak {

	constexpr size_t c_str_len(const char *str) {
		const char *c = str;
		while (*c) {
			c++;
		}
		return reinterpret_cast<uintptr_t>(c) - reinterpret_cast<uintptr_t>(str);
	}

	template<typename T>
	struct Slice {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef T value_type; 

		constexpr Slice() = default;
		constexpr Slice(T *_data, size_t _size) : data{ _data }, size{ _size } {}
		template<size_t C, typename U = T, typename = std::enable_if_t<!std::is_same_v<U, char>>>
		constexpr Slice(T (&array)[C]) : data{ &array[0] }, size{ C } {}
		template<typename U = T, typename = std::enable_if_t<std::is_same_v<U, char>>>
		constexpr Slice(const char *cstr) : data{ const_cast<char*>(cstr) }, size{ c_str_len(cstr) } {}

		constexpr size_t find(const T& v, size_t start = 0) const {
			if (!data) { return npos; }
			for (size_t i = start; i < size; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return npos;
		}

		Slice<T> clone(IAllocator *allocator) const {
			if (!size) { return {}; }
			auto mem = static_cast<std::remove_const_t<T>*>(allocator->alloc(size));
			std::memcpy(mem, data, size);
			return { mem, size };
		}

		constexpr T& operator[](size_t idx) { return data[idx]; }
		constexpr const T& operator[](size_t idx) const { return data[idx]; }

		inline T* begin() const { return data; }
		inline T* end() const { return data + size; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + size; }

		T *data = nullptr;
		size_t size = 0;
	};

	template<typename T>
	constexpr bool operator==(const Slice<T>& lhs, const Slice<T>& rhs) {
		if (lhs.size != rhs.size) { return false; }	
		if (lhs.size == 0) { return true; }
		for (size_t i = 0; i < lhs.size; i++) {
			if (lhs[i] != rhs[i]) {
				return false;
			}
		}
		return true;
	}

	template<typename T>
	constexpr bool operator!=(const Slice<T>& lhs, const Slice<T>& rhs) {
		return !(lhs == rhs);
	}

}

