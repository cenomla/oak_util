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

	struct _reflect(oak::catagory::none) String {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		constexpr String() = default;
		constexpr String(char *_data, size_t _size) : data{ _data }, size{ _size } {}
		constexpr String(const Array<char>& other) : data{ const_cast<char*>(other.data) }, size{ other.size } {}
		constexpr String(const Slice<char>& other) : data{ const_cast<char*>(other.data) }, size{ other.size } {}
		constexpr String(const char *cstr) : data{ const_cast<char*>(cstr) }, size{ c_str_len(cstr) } {}

		String substr(size_t start, size_t end = npos) const;
		size_t find(char c, size_t start = 0) const;
		size_t find_first_of(String delimeters, size_t start = 0) const;
		size_t find_first_not_of(String delimeters, size_t start = 0) const;
		size_t find_last_of(String delimeters, size_t start = 0) const;
		size_t find_string(String str, size_t start = 0) const;
		void splitstr(String delimeters, Array<String>& tokens) const;
		bool is_c_str() const;
		const char* as_c_str() const;
		const char* make_c_str(IAllocator *allocator) const;
		String clone(IAllocator *allocator) const;

		constexpr char& operator[](size_t idx) { return data[idx]; }
		constexpr const char& operator[](size_t idx) const { return data[idx]; }

		inline char* begin() const { return data; }
		inline char* end() const { return data + size; }
		inline const char* cbegin() const { return data; }
		inline const char* cend() const { return data + size; }

		char *data = nullptr;
		size_t size = 0;
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

