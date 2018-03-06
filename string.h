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
		using Slice::operator=;

		constexpr String() = default;
		constexpr String(const Slice<const char>& other) : Slice{ other } {}
		constexpr String(Slice<const char>&& other) : Slice{ std::move(other) } {}

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

}

