#include "oak_util/string.h"

#include <cstring>

#include "oak_util/allocator.h"
#include "oak_util/memory.h"

namespace oak {

	String substr(const String str, int64_t start, int64_t end) {
		//bounds checking
		if (end == -1) { end = str.count; }
		return String{ str.data + start, end - start };
	}

	int64_t find_first_of(const String str, const String delimeters, int64_t start) {
		for (auto i = start; i < str.count; i++) {
			for (int64_t j = 0; j < delimeters.count; j++) {
				if (str.data[i] == delimeters.data[j]) {
					return i;
				}
			}
		}
		return -1;
	}

	int64_t find_first_not_of(const String str, const String delimeters, int64_t start) {
		bool found;
		for (auto i = start; i < str.count; i++) {
			found = false;
			for (int64_t j = 0; j < delimeters.count; j++) {
				if (str.data[i] == delimeters.data[j]) {
					found = true;
					break;
				}
			}
			if (!found) {
				return i;
			}
		}
		return -1;
	}

	int64_t find_last_of(const String str, const String delimeters, int64_t start) {
		for (auto i = str.count; i > start; i--) {
			for (int64_t j = 0; j < delimeters.count; j++) {
				if (str.data[i - 1] == delimeters.data[j]) {
					return i - 1;
				}
			}
		}
		return -1;
	}

	int64_t find_string(const String str, const String value, int64_t start) {
		if (str.count < value.count) { return -1; }
		for (auto i = start; i <= str.count - value.count; i++) {
			if (value == String{ str.data + i, value.count }) {
				return i;
			}
		}
		return -1;
	}

	Slice<String> splitstr(const String str, const String delimeters) {
		int64_t tokenCapacity = 64;

		Slice<String> tokens;
		tokens.data = allocate<String>(temporaryMemory, tokenCapacity);

		int64_t first = 0, last = 0;

		while (last != -1) {
			first = find_first_not_of(str, delimeters, first);
			if (first == -1) { break; } // Rest of string is entirely delimeters
			last = find_first_of(str, delimeters, first);
			if (tokens.count == tokenCapacity) {
				tokenCapacity *= 2;
				auto ndata = allocate<String>(temporaryMemory, tokenCapacity);
				memcpy(ndata, tokens.data, tokens.count);
				tokens.data = ndata;
			}
			tokens[tokens.count++] = substr(str, first, last);

			first = last + 1;
		}

		return tokens;
	}

	bool is_c_str(const String str) {
		return str.data + str.count == 0;
	}

	const char* as_c_str(const String str) {
		if (!str.count) { return ""; }
		if (is_c_str(str)) { return str.data; }
		auto cstr = allocate<char>(temporaryMemory, str.count + 1);
		std::memmove(cstr, str.data, str.count);
		cstr[str.count] = 0;
		return cstr;
	}

	String copy_str(MemoryArena *arena, const String str) {
		String string;
		string.count = str.count;
		string.data = allocate_structs<char>(arena, string.count);
		std::memcpy(string.data, str.data, string.count);
		return string;
	}

	void reverse(String& str) {
		if (str.count < 2) { return; }
		for (int64_t i = 0; i < str.count >> 1; i++) {
			auto tmp = str[i];
			str[i] = str[str.count - 1 - i];
			str[str.count - 1 - i] = tmp;
		}
	}

}

