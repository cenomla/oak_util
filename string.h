#pragma once

#include <cstddef>
#include <algorithm>

#include "osig_defs.h"
#include "array.h"
#include "slice.h"

namespace oak {

	struct IAllocator;

	using String = Slice<char>;

	String substr(const String str, size_t start, size_t end = String::npos);
	size_t find_first_of(const String str, String delimeters, size_t start = 0);
	size_t find_first_not_of(const String str, String delimeters, size_t start = 0);
	size_t find_last_of(const String str, String delimeters, size_t start = 0);
	size_t find_string(const String str, String value, size_t start = 0);
	void splitstr(const String str, String delimeters, Array<String>& tokens);
	bool is_c_str(const String str);
	const char* as_c_str(const String str);
	const char* make_c_str(const String str, IAllocator *allocator);

	constexpr size_t hash(const String& str) {
		size_t hash = 0;
		
		for (size_t i = 0; i < str.size; i++) {
			hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
		}

		return hash;
	}

	constexpr bool operator==(const String& lhs, const char *rhs) {
		return lhs == String{ rhs };
	}

	constexpr bool operator!=(const String& lhs, const char *rhs) {
		return !(lhs == String{ rhs });
	}

}

