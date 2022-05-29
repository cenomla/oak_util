#define OAK_UTIL_EXPORT_SYMBOLS

#include <oak_util/fmt.h>

#include <cassert>
#include <cctype>
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
			case FmtKind::HEX: case FmtKind::HEX_CAP: base = 16; break;
			default: base = 10; break;
		}
		return base;
	}

	char const* choose_snprintf_float_fmt_string(FmtKind const fmtKind) noexcept {
		char const* str;
		switch (fmtKind) {
			case FmtKind::DEFAULT: case FmtKind::DEC: str = "%g"; break;
			case FmtKind::HEX: str = "%a"; break;
			case FmtKind::HEX_CAP: str = "%A"; break;
			case FmtKind::EXP: str = "%e"; break;
			default: str = ""; break;
		}

		return str;
	}

	char const* choose_snprintf_double_fmt_string(FmtKind const fmtKind) noexcept {
		char const* str;
		switch (fmtKind) {
			case FmtKind::DEFAULT: case FmtKind::DEC: str = "%lg"; break;
			case FmtKind::HEX: str = "%la"; break;
			case FmtKind::HEX_CAP: str = "%lA"; break;
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
		auto letter = fmtKind == FmtKind::HEX_CAP ? 'A' : 'a';
		auto str = allocate<char>(allocator, 66);
		int idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = c > 9 ? letter + c - 10 : '0' + c;
			v /= base;
		} while (v > 0);

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
		auto letter = fmtKind == FmtKind::HEX_CAP ? 'A' : 'a';
		auto str = allocate<char>(allocator, 66);
		bool neg = false;
		if (v < 0) {
			neg = true;
			v = -v;
		}
		int idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = c > 9 ? letter + c - 10 : '0' + c;
			v /= base;
		} while (v > 0);

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

	String to_str(Allocator *allocator, char const *const v, FmtKind fmtKind) {
		return to_str(allocator, String{ v }, fmtKind);
	}

	String to_str(Allocator *allocator, unsigned char const *const v, FmtKind fmtKind) {
		return to_str(allocator, String{ reinterpret_cast<char const *>(v) }, fmtKind);
	}

	String to_str(Allocator *allocator, String const str, FmtKind fmtKind) {
		switch (fmtKind) {
		case FmtKind::LOWER:
			{
				auto nstr = allocate<char>(allocator, str.count);
				for (i64 i = 0; i < str.count; ++i)
					nstr[i] = tolower(str[i]);
				return String{ nstr, str.count };
			}
			break;
		case FmtKind::UPPER:
			{
				auto nstr = allocate<char>(allocator, str.count);
				for (i64 i = 0; i < str.count; ++i)
					nstr[i] = toupper(str[i]);
				return String{ nstr, str.count };
			}
			break;
		case FmtKind::DEFAULT:
		default:
			return str;
			break;
		}
	}

	void FileBuffer::write(void const *data, usize size) {
		std::fwrite(data, 1, size, file);
		std::fflush(file);
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
		assert(buffer->count + static_cast<i64>(size) <= capacity);
		std::memcpy(buffer->data + buffer->count, data, size);
		buffer->count += size;
	}

	void ArrayBuffer::write(void const *data, usize size) {
		assert(*count + static_cast<i64>(size) <= capacity);
		std::memcpy(buffer + *count, data, size);
		*count += size;
	}

}

