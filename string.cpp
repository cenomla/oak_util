#include "string.h"

#include <cstring>

#include "array.h"
#include "allocator.h"

namespace oak {

	String::String(char *data, size_t size) : data{ data }, size{ size } {}

	String::String(const Array<char>& data) : data{ data.data }, size{ data.size } {}

	String String::substr(size_t start, size_t end) const {
		//bounds checking 
		if (end == npos) { end = size; }
		return String{ data + start, end - start }; 
	}

	size_t String::find_char(char c, size_t start) const {
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

	void String::splitstr(String delimeters, Array<String>& tokens) const {
		size_t prev = 0, pos;
		do {
			pos = find_first_of(delimeters, prev);
			if (pos > prev) {
				tokens.push(substr(prev, pos-prev));
			}
			prev = pos + 1;
		} while(pos != npos);
	}

	bool String::is_c_str() const {
		//TODO: can this segfault?
		return data + size == 0;
	}

	static Array<char> cStringStorage{ &proxyAlloc };

	const char* String::as_c_str() const {
		if (is_c_str()) { return data; }
		if (cStringStorage.size <= size) {
			cStringStorage.resize(size + 1, 0);
		}
		std::memcpy(cStringStorage.data, data, size);
		cStringStorage[size] = 0;
		return cStringStorage.data;
	}

	bool operator==(const String& lhs, const String& rhs) {
		if (lhs.size != rhs.size) { return false; }	
		if (lhs.size == 0) { return true; }
		for (size_t i = 0; i < lhs.size; i++) {
			if (lhs.data[i] != rhs.data[i]) {
				return false;
			}
		}
		return true;
	}

}
