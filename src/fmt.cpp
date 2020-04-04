#include <oak_util/fmt.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace oak {

namespace {
	i64 choose_base(FmtKind const fmtKind) noexcept {
		i64 base;
		switch (fmtKind) {
			case FmtKind::BIN: base = 2; break;
			case FmtKind::OCT: base = 8; break;
			case FmtKind::HEX: base = 16; break;
			default: base = 10; break;
		}
		return base;
	}

	void put_base_prefix(FmtKind const fmtKind, char *const str, int *const idx) {
		switch (fmtKind) {
			case FmtKind::BIN: str[(*idx)++] = 'b'; str[(*idx)++] = '0'; break;
			case FmtKind::OCT: str[(*idx)++] = '0'; break;
			case FmtKind::HEX: str[(*idx)++] = 'x'; str[(*idx)++] = '0'; break;
			default: break;
		}
	}

	char const* choose_snprintf_float_fmt_string(FmtKind const fmtKind) noexcept {
		char const* str;
		switch (fmtKind) {
			case FmtKind::DEFAULT: case FmtKind::DEC: str = "%f"; break;
			case FmtKind::HEX: str = "%a"; break;
			case FmtKind::EXP: str = "%e"; break;
			default: str = ""; break;
		}

		return str;
	}

	char const* choose_snprintf_double_fmt_string(FmtKind const fmtKind) noexcept {
		char const* str;
		switch (fmtKind) {
			case FmtKind::DEFAULT: case FmtKind::DEC: str = "%lf"; break;
			case FmtKind::HEX: str = "%la"; break;
			case FmtKind::EXP: str = "%le"; break;
			default: str = ""; break;
		}

		return str;
	}

}

	String to_str(Allocator *const allocator, char const v, FmtKind) {
		auto str = allocate<char>(allocator, 1);
		str[0] = v;
		return String{ str, 1 };
	}

	String to_str(Allocator *const allocator, u32 const v, FmtKind const fmtKind) {
		return to_str(allocator, static_cast<u64>(v), fmtKind);
	}

	String to_str(Allocator *const allocator, u64 v, FmtKind const fmtKind) {
		auto base = choose_base(fmtKind);
		auto str = allocate<char>(allocator, 66);
		int idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = c > 9 ? 'a' + c - 10 : '0' + c;
			v /= base;
		} while (v > 0);

		put_base_prefix(fmtKind, str, &idx);

		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

#ifdef USIZE_OVERRIDE_NEEDED
	String to_str(Allocator *const allocator, usize v, FmtKind const fmtKind) {
		return to_str(allocator, static_cast<u64>(v), fmtKind);
	}
#endif

	String to_str(Allocator *const allocator, i32 const v, FmtKind const fmtKind) {
		return to_str(allocator, static_cast<i64>(v), fmtKind);
	}

	String to_str(Allocator *const allocator, i64 v, FmtKind const fmtKind) {
		auto base = choose_base(fmtKind);
		auto str = allocate<char>(allocator, 66);
		bool neg = false;
		if (v < 0) {
			neg = true;
			v = -v;
		}
		int idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = c > 9 ? 'a' + c - 10 : '0' + c;
			v /= base;
		} while (v > 0);

		put_base_prefix(fmtKind, str, &idx);
		if (neg) {
			str[idx++] = '-';
		}

		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(Allocator *const allocator, f32 const v, FmtKind const fmtKind) {
		constexpr usize bufSize = 32;
		auto str = make<char>(allocator, bufSize);
		std::snprintf(str, bufSize, choose_snprintf_float_fmt_string(fmtKind), v);
		return str;
	}

	String to_str(Allocator *const allocator, f64 const v, FmtKind const fmtKind) {
		constexpr usize bufSize = 32;
		auto str = make<char>(allocator, bufSize);
		std::snprintf(str, bufSize, choose_snprintf_double_fmt_string(fmtKind), v);
		return str;
	}

	String to_str(Allocator *, char const *const v, FmtKind) {
		return String{ v };
	}

	String to_str(Allocator *, unsigned char const *const v, FmtKind) {
		return String{ reinterpret_cast<char const *>(v) };
	}

	String to_str(Allocator *, String const str, FmtKind) {
		return str;
	}

	void FileBuffer::write(void const *data, usize size) {
		std::fwrite(data, 1, size, file);
	}

	void StringBuffer::write(void const *data, usize size) {
		assert(size <= buffer->count - pos);
		std::memcpy(buffer->data + pos, data, size);
		pos += size;
	}

	void StringBuffer::resize(usize size) {
		size += pos;
		auto ndata = make<char>(allocator, size);
		if (buffer->data) {
			std::memcpy(ndata, buffer->data, buffer->count);
		}
		buffer->data = ndata;
		buffer->count = size;
	}

	void SliceBuffer::write(void const *data, usize size) {
		assert(buffer->count + size <= capacity);
		std::memcpy(buffer->data + buffer->count, data, size);
		buffer->count += size;
	}

}

