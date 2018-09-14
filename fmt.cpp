#include "fmt.h"

#include <cassert>
#include <cstdio>

namespace oak::detail {

	size_t to_str_size(char v) {
		return 1;
	}

	size_t to_str_size(int32_t v) {
		return to_str_size(static_cast<int64_t>(v));
	}

	size_t to_str_size(int64_t v) {
		size_t t = 0;
		if (v < 0) {
			t++;
			v = -v;
		}
		return t + to_str_size(static_cast<uint64_t>(v));
	}

	size_t to_str_size(uint32_t v) {
		return to_str_size(static_cast<uint64_t>(v));
	}

	size_t to_str_size(uint64_t v) {
		uint64_t c = 10;
		size_t t = 1;
		while (c <= v) {
			t++;
			c *= 10;
		}
		return t;
	}

	size_t to_str_size(float v) {
		auto str = make_structs<char>(temporaryMemory, 32, char{ 0 });
		std::sprintf(str, "%f", v);
		return c_str_len(str);
	}

	size_t to_str_size(double v) {
		auto str = make_structs<char>(temporaryMemory, 32, char{ 0 });
		std::sprintf(str, "%lf", v);
		return c_str_len(str);
	}

	size_t to_str_size(const char *v) {
		return c_str_len(v);
	}

	size_t to_str_size(const unsigned char *v) {
		return c_str_len(reinterpret_cast<const char*>(v));
	}

	size_t to_str_size(String v) {
		return v.count;
	}

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
			auto nv = v / 10;
			str[idx++] = '0' + static_cast<char>(v - (nv * 10));
			v = nv;
		} while (v > 0);
		return { str, idx };
	}

	String to_str(int32_t v) {
		return to_str(static_cast<int64_t>(v));
	}

	String to_str(int64_t v) {
		auto str = make_structs<char>(temporaryMemory, 32, char{ 0 });
		int idx = 0;
		if (v < 0) {
			str[idx++] = '-';
			v = -v;
		}
		do {
			auto nv = v / 10;
			str[idx++] = '0' + static_cast<char>(v - (nv * 10));
			v = nv;
		} while (v > 0);
		return { str, idx };
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

