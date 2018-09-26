#pragma once

#include <cinttypes>
#include <cstddef>

#include "slice.h"
#include "hash.h"

namespace oak {

	using String = Slice<char>;

	String substr(const String str, int64_t start, int64_t end = -1);
	int64_t find_first_of(const String str, const String delimeters, int64_t start = 0);
	int64_t find_first_not_of(const String str, const String delimeters, int64_t start = 0);
	int64_t find_last_of(const String str, const String delimeters, int64_t start = 0);
	int64_t find_string(const String str, const String value, int64_t start = 0);
	Slice<String> splitstr(const String str, const String delimeters);
	bool is_c_str(const String str);
	const char* as_c_str(const String str);
	String copy_str(const String str);
	void reverse(String& str);

	template<>
	struct HashFunc<String> {
		constexpr size_t operator()(const String& str) const {
			size_t hash = 0;

			for (auto i = 0ll; i < str.count; i++) {
				hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
			}

			return hash + 1;
		}
	};

	constexpr bool operator==(const String& lhs, const char *rhs) {
		return lhs == String{ rhs };
	}

	constexpr bool operator!=(const String& lhs, const char *rhs) {
		return !(lhs == String{ rhs });
	}

}

