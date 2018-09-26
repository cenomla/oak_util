#pragma once

#include <type_traits>
#include <cinttypes>

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
		constexpr Slice(T *data_, int64_t count_) : data{ data_ }, count{ count_ } {}
		template<int64_t C, typename U = T, typename = std::enable_if_t<!std::is_same_v<U, char>>>
		constexpr Slice(T (&array)[C]) : data{ &array[0] }, count{ static_cast<int64_t>(C) } {}
		template<typename U = T, typename = std::enable_if_t<std::is_same_v<U, char>>>
		constexpr Slice(const char *cstr) : data{ const_cast<char*>(cstr) }, count{ c_str_len(cstr) } {}

		constexpr int64_t find(const T& v, int64_t start = 0) const {
			if (!data) { return -1; }
			for (auto i = start; i < count; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return -1;
		}

		constexpr T& operator[](int64_t idx) { return data[idx]; }
		constexpr const T& operator[](int64_t idx) const { return data[idx]; }

		inline T* begin() const { return data; }
		inline T* end() const { return data + count; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + count; }

		T *data = nullptr;
		int64_t count = 0;
	};

	template<typename T>
	constexpr bool operator==(const Slice<T>& lhs, const Slice<T>& rhs) {
		if (lhs.count != rhs.count) { return false; }
		if (lhs.count == 0 || lhs.data == rhs.data) { return true; }
		// This next early exit check was implemented for the use of string pools.
		// However if the slices are the same size and point to the same memory they must contain the same contents
		// so this is check shouldn't break anything even for non interned slices.
		// -coleman 6.1.2018
		for (int64_t i = 0; i < lhs.count; i++) {
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

