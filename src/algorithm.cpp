#include <oak_util/algorithm.h>

#include <cstring>

#include <oak_util/memory.h>

namespace oak {

	bool is_c_str(String const str) {
		return str.data + str.count == 0;
	}

	char const* as_c_str(String const str) {
		if (!str.count) { return ""; }
		if (is_c_str(str)) { return str.data; }
		auto cstr = allocate<char>(&temporaryMemory, str.count + 1);
		std::memmove(cstr, str.data, str.count);
		cstr[str.count] = 0;
		return cstr;
	}

	String copy_str(Allocator *allocator, String const str) {
		String string;
		string.count = str.count;
		string.data = allocate<char>(allocator, string.count);
		std::memcpy(string.data, str.data, string.count);
		return string;
	}

}
