#pragma once

#include <cstddef>
#include <cinttypes>
#include <cstring>
#include <type_traits>

#include "allocator.h"

namespace oak {

	constexpr int64_t c_str_len(const char *str) {
		auto c = str;
		while (*c) {
			c++;
		}
		return reinterpret_cast<int64_t>(c - str);
	}

	template<typename T>
	struct Slice {
		typedef T value_type;

		constexpr Slice() = default;
		constexpr Slice(T *_data, int64_t _size) : data{ _data }, size{ _size } {}
		template<int64_t C, typename U = T, typename = std::enable_if_t<!std::is_same_v<U, char>>>
		constexpr Slice(T (&array)[C]) : data{ &array[0] }, size{ static_cast<int64_t>(C) } {}
		template<typename U = T, typename = std::enable_if_t<std::is_same_v<U, char>>>
		constexpr Slice(const char *cstr) : data{ const_cast<char*>(cstr) }, size{ c_str_len(cstr) } {}

		constexpr int64_t find(const T& v, int64_t start = 0) const {
			if (!data) { return -1; }
			for (auto i = start; i < size; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return -1;
		}

		Slice<T> clone(IAllocator *allocator) const {
			if (!size) { return {}; }
			auto mem = static_cast<std::remove_const_t<T>*>(allocator->alloc(size));
			std::memcpy(mem, data, size);
			return { mem, size };
		}

		constexpr T& operator[](int64_t idx) { return data[idx]; }
		constexpr const T& operator[](int64_t idx) const { return data[idx]; }

		inline T* begin() const { return data; }
		inline T* end() const { return data + size; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + size; }

		T *data = nullptr;
		int64_t size = 0;
	};

	template<typename T>
	constexpr bool operator==(const Slice<T>& lhs, const Slice<T>& rhs) {
		if (lhs.size != rhs.size) { return false; }
		if (lhs.size == 0) { return true; }
		// This next early exit check was implemented for the use of string pools.
		// However if the slices are the same size and point to the same memory they must contain the same contents
		// so this is check shouldn't break anything even for non interned slices.
		// -coleman 6.1.2018
		if (lhs.data == rhs.data) { return true; }
		for (int64_t i = 0; i < lhs.size; i++) {
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

