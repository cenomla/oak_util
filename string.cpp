#include "string.h"

#include <cstring>
#include <cstdio>

#include "allocator.h"

namespace oak {

	String String::substr(size_t start, size_t end) const {
		//bounds checking 
		if (end == npos) { end = size; }
		return String{ data + start, end - start }; 
	}

	size_t String::find(char c, size_t start) const {
		if (!data) { return npos; }
		for (size_t i = start; i < size; i++) {
			if (data[i] == c) {
				return i;
			}
		}
		return npos;
	}

	size_t String::find_first_of(String delimeters, size_t start) const {
		for (size_t i = start; i < size; i++) {
			for (size_t j = 0; j < delimeters.size; j++) {
				if (data[i] == delimeters.data[j]) {
					return i;
				}
			}
		}
		return npos;
	}

	size_t String::find_first_not_of(String delimeters, size_t start) const {
		bool found;
		for (size_t i = start; i < size; i++) {
			found = false;
			for (size_t j = 0; j < delimeters.size; j++) {
				if (data[i] == delimeters.data[j]) {
					found = true;
					break;
				}
			}
			if (!found) {
				return i;
			}
		}
		return npos;
	}

	size_t String::find_last_of(String delimeters, size_t start) const {
		for (size_t i = size; i > start; i--) {
			for (size_t j = 0; j < delimeters.size; j++) {
				if (data[i - 1] == delimeters.data[j]) {
					return i - 1;
				}
			}
		}
		return npos;
	}

	size_t String::find_string(String str, size_t start) const {
		if (size < str.size) { return npos; }
		for (size_t i = start; i <= size - str.size; i++) {
			if (str == String{ data + i, str.size }) {
				return i;
			}
		}
		return npos;
	}

	void String::splitstr(String delimeters, Array<String>& tokens) const {
		size_t first = 0, last = 0;
		do {
			first = find_first_not_of(delimeters, first);
			if (first == npos) { break; }
			last = find_first_of(delimeters, first);
			if (last > first) {
				tokens.push(substr(first, last));
			}
			first = last + 1;
		} while(last != npos);
	}

	bool String::is_c_str() const {
		//TODO: can this segfault?
		return data + size == 0;
	}

	const char* String::as_c_str() const {
		static char cstr[2048]{ 0 };
		//if (is_c_str()) { return data; }
		std::memmove(cstr, data, size);
		cstr[size] = 0;
		return cstr;
	}

	const char* String::make_c_str(IAllocator *allocator) const {
		if (!size) { return ""; }
		auto cstr = static_cast<char*>(allocator->alloc(size + 1));
		std::memcpy(cstr, data, size);
		cstr[size] = 0;
		return cstr;
	}

	String String::clone(IAllocator *allocator) const {
		if (!size) { return {}; }
		auto mem = static_cast<char*>(allocator->alloc(size));
		std::memcpy(mem, data, size);
		return { mem, size };
	}

}

