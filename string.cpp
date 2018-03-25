#include "string.h"

#include <cstring>
#include <cstdio>

#include "allocator.h"

namespace oak {

	String substr(const String str, size_t start, size_t end) {
		//bounds checking
		if (end == String::npos) { end = str.size; }
		return String{ str.data + start, end - start };
	}

	size_t find_first_of(const String str, String delimeters, size_t start) {
		for (size_t i = start; i < str.size; i++) {
			for (size_t j = 0; j < delimeters.size; j++) {
				if (str.data[i] == delimeters.data[j]) {
					return i;
				}
			}
		}
		return String::npos;
	}

	size_t find_first_not_of(const String str, String delimeters, size_t start) {
		bool found;
		for (size_t i = start; i < str.size; i++) {
			found = false;
			for (size_t j = 0; j < delimeters.size; j++) {
				if (str.data[i] == delimeters.data[j]) {
					found = true;
					break;
				}
			}
			if (!found) {
				return i;
			}
		}
		return String::npos;
	}

	size_t find_last_of(const String str, String delimeters, size_t start) {
		for (size_t i = str.size; i > start; i--) {
			for (size_t j = 0; j < delimeters.size; j++) {
				if (str.data[i - 1] == delimeters.data[j]) {
					return i - 1;
				}
			}
		}
		return String::npos;
	}

	size_t find_string(const String str, String value, size_t start) {
		if (str.size < value.size) { return String::npos; }
		for (size_t i = start; i <= str.size - value.size; i++) {
			if (value == String{ str.data + i, value.size }) {
				return i;
			}
		}
		return String::npos;
	}

	void splitstr(const String str, String delimeters, Array<String>& tokens) {
		size_t first = 0, last = 0;
		do {
			first = find_first_not_of(str, delimeters, first);
			if (first == String::npos) { break; }
			last = find_first_of(str, delimeters, first);
			if (last > first) {
				tokens.push(substr(str, first, last));
			}
			first = last + 1;
		} while(last != String::npos);
	}

	bool is_c_str(const String str) {
		//TODO: can this segfault?
		return str.data + str.size == 0;
	}

	const char* as_c_str(const String str) {
		static char cstr[2048]{ 0 };
		//if (is_c_str()) { return data; }
		std::memmove(cstr, str.data, str.size);
		cstr[str.size] = 0;
		return cstr;
	}

	const char* make_c_str(const String str, IAllocator *allocator) {
		if (!str.size) { return ""; }
		auto cstr = static_cast<char*>(allocator->alloc(str.size + 1));
		std::memcpy(cstr, str.data, str.size);
		cstr[str.size] = 0;
		return cstr;
	}

}

