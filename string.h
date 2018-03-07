#pragma once

#include <cstddef>
#include <algorithm>

#include "osig_defs.h"
#include "array.h"
#include "slice.h"

namespace oak {

	struct IAllocator;

	constexpr size_t c_str_len(const char *str) {
		const char *c = str;
		while (*c) {
			c++;
		}
		return static_cast<size_t>(c - str);
	}

	struct _reflect(oak::catagory::none) String : Slice<const char> {

		using Slice::Slice;

		constexpr String() = default;
		constexpr String(const Slice<const char>& other) : Slice{ other } {}
		constexpr String(Slice<const char>&& other) : Slice{ std::move(other) } {}
		constexpr String(const char *cstr) : Slice{ cstr, c_str_len(cstr) } {}
		template<size_t C>
		constexpr String(const char (&array)[C]) : Slice{ &array[0], C - 1 } {}

		String substr(size_t start, size_t end = npos) const;
		size_t find_char(char c, size_t start = 0) const;
		size_t find_first_of(String delimeters, size_t start = 0) const;
		size_t find_first_not_of(String delimeters, size_t start = 0) const;
		size_t find_last_of(String delimeters, size_t start = 0) const;
		size_t find_string(String str, size_t start = 0) const;
		void splitstr(String delimeters, Array<String>& tokens) const;
		bool is_c_str() const;
		const char* as_c_str() const;
		const char* make_c_str(IAllocator *allocator) const;
	};

	constexpr size_t hash(const String& str) {
		size_t hash = 0;
		
		for (size_t i = 0; i < str.size; i++) {
			hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
		}

		return hash;
	}

	constexpr bool operator==(const String& lhs, const String& rhs) {
		if (lhs.size != rhs.size) { return false; }	
		if (lhs.size == 0) { return true; }
		for (size_t i = 0; i < lhs.size; i++) {
			if (lhs[i] != rhs[i]) {
				return false;
			}
		}
		return true;
	}

	constexpr bool operator!=(const String& lhs, const String& rhs) {
		return !(lhs == rhs);
	}

}

