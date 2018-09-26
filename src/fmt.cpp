#include "fmt.h"

#include <cassert>
#include <cstdio>

namespace oak::detail {

	String to_str(char v) {
		auto str = allocate_structs<char>(temporaryMemory, 1);
		str[0] = v;
		return String{ str, 1 };
	}

	String to_str(uint32_t v) {
		return to_str(static_cast<uint64_t>(v));
	}

	String to_str(uint64_t v) {
		auto str = allocate_structs<char>(temporaryMemory, 32);
		int idx = 0;
		do {
			str[idx++] = '0' + static_cast<char>(v % 10);
			v /= 10;
		} while (v > 0);
		String string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(int32_t v) {
		return to_str(static_cast<int64_t>(v));
	}

	String to_str(int64_t v) {
		auto str = allocate_structs<char>(temporaryMemory, 32);
		bool neg = false;
		if (v < 0) {
			neg = true;
			v = -v;
		}
		int idx = 0;
		do {
			str[idx++] = '0' + static_cast<char>(v % 10);
			v /= 10;
		} while (v > 0);

		if (neg) {
			str[idx++] = '-';
		}
		String string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(float v) {
		auto str = make_structs<char>(temporaryMemory, 32, char{ 0 });
		std::sprintf(str, "%f", v);
		return str;
	}

	String to_str(double v) {
		auto str = make_structs<char>(temporaryMemory, 32, char{ 0 });
		std::sprintf(str, "%lf", v);
		return str;
	}

	String to_str(const char *v) {
		return String{ v };
	}

	String to_str(const unsigned char *v) {
		return String{ reinterpret_cast<const char *>(v) };
	}

	String to_str(String str) {
		return str;
	}

}

namespace oak {

	void FileBuffer::write(const void *data, size_t size) {
		std::fwrite(data, 1, size, file);
	}

	void StringBuffer::write(const void *data, size_t size) {
		assert(size <= buffer->count - pos);
		std::memcpy(buffer->data + pos, data, size);
		pos += size;
	}

	void StringBuffer::resize(size_t size) {
		size += pos;
		auto ndata = make_structs<char>(arena, size, char{ 0 });
		if (buffer->data) {
			std::memcpy(ndata, buffer->data, buffer->count);
		}
		buffer->data = ndata;
		buffer->count = size;
	}

}

