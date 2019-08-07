#include "oak_util/fmt.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace oak {

	String to_str(char v, FmtKind) {
		auto str = allocate<char>(&temporaryMemory, 1);
		str[0] = v;
		return String{ str, 1 };
	}

	String to_str(u32 v, FmtKind fmtKind) {
		return to_str(static_cast<u64>(v), fmtKind);
	}

	String to_str(u64 v, FmtKind) {
		auto str = allocate<char>(&temporaryMemory, 32);
		int idx = 0;
		do {
			str[idx++] = '0' + static_cast<char>(v % 10);
			v /= 10;
		} while (v > 0);
		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(i32 v, FmtKind fmtKind) {
		return to_str(static_cast<i64>(v), fmtKind);
	}

	String to_str(i64 v, FmtKind) {
		auto str = allocate<char>(&temporaryMemory, 32);
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
		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(f32 v, FmtKind) {
		auto str = make<char>(&temporaryMemory, 32);
		std::sprintf(str, "%f", v);
		return str;
	}

	String to_str(f64 v, FmtKind) {
		auto str = make<char>(&temporaryMemory, 32);
		std::sprintf(str, "%lf", v);
		return str;
	}

	String to_str(char const *v, FmtKind) {
		return String{ v };
	}

	String to_str(unsigned char const *v, FmtKind) {
		return String{ reinterpret_cast<char const *>(v) };
	}

	String to_str(String str, FmtKind) {
		return str;
	}

	void FileBuffer::write(void const *data, u64 size) {
		std::fwrite(data, 1, size, file);
	}

	void StringBuffer::write(void const *data, u64 size) {
		assert(size <= buffer->count - pos);
		std::memcpy(buffer->data + pos, data, size);
		pos += size;
	}

	void StringBuffer::resize(u64 size) {
		size += pos;
		auto ndata = make<char>(allocator, size);
		if (buffer->data) {
			std::memcpy(ndata, buffer->data, buffer->count);
		}
		buffer->data = ndata;
		buffer->count = size;
	}

}

