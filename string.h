#pragma once

#include <cstddef>

namespace oak {

	constexpr size_t c_str_len(const char *str) {
		const char *c = str;
		while (*c) {
			c++;
		}
		return static_cast<size_t>(c - str);
	}

	template<class T>
	struct Array;

	struct String {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		constexpr String() = default;
		constexpr String(const char *str) :
			data{ const_cast<char*>(str) }, size{ c_str_len(str) } {}

		String(char *data, size_t size);
		String(const Array<char>& data);

		String substr(size_t start, size_t end = npos) const;
		size_t find_char(char c, size_t start = 0) const;
		size_t find_first_of(String delimeters, size_t start = 0) const;
		size_t find_first_not_of(String delimeters, size_t start = 0) const;
		void splitstr(String delimeters, Array<String>& tokens) const;
		bool is_c_str() const;
		const char* as_c_str() const;

		inline char* begin() { return data; }
		inline char* end() { return data + size; }

		char *data = nullptr;
		size_t size = 0;
	};

	constexpr size_t hash_string(const String& str) {
		size_t hash = 0;
		
		for (size_t i = 0; i < str.size; i++) {
			hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
		}

		return hash;
	}

	bool operator==(const String& lhs, const String& rhs);
	inline bool operator!=(const String& lhs, const String& rhs) { return !operator==(lhs, rhs); }
}

