#define OAK_UTIL_EXPORT_SYMBOLS
#include <oak_util/algorithm.h>

#include <string.h>

#include <oak_util/memory.h>

namespace oak {

	bool is_c_str(String str) noexcept {
		return str.data[str.count] == 0;
	}

	char const* as_c_str(Allocator *allocator, String str) noexcept {
		if (!str.count)
			return "";

		if (is_c_str(str))
			return str.data;

		auto cstr = allocate<char>(allocator, str.count + 1);
		memmove(cstr, str.data, str.count);
		cstr[str.count] = 0;

		return cstr;
	}

	String copy_str(Allocator *allocator, String string, bool isCStr) noexcept {
		auto nData = allocate<char>(allocator, string.count + isCStr);
		memcpy(nData, string.data, string.count + isCStr);
		return { nData, string.count };
	}

}
