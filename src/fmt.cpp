#define OAK_UTIL_EXPORT_SYMBOLS

#include <oak_util/fmt.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace oak {

namespace {
	i64 choose_base(FmtKind fmtKind) noexcept {
		i64 base;
		switch (fmtKind) {
			case FmtKind::BIN: base = 2; break;
			case FmtKind::OCT: base = 8; break;
			case FmtKind::HEX: case FmtKind::HEX_CAP: base = 16; break;
			default: base = 10; break;
		}
		return base;
	}

	char const* choose_snprintf_float_fmt_string(FmtKind fmtKind) noexcept {
		char const* str;
		switch (fmtKind) {
			case FmtKind::DEFAULT: str = "%.*g"; break;
			case FmtKind::DEC: str = "%.*f"; break;
			case FmtKind::HEX: str = "%.*a"; break;
			case FmtKind::HEX_CAP: str = "%.*A"; break;
			case FmtKind::EXP: str = "%.*e"; break;
			default: str = ""; break;
		}

		return str;
	}

	void sb_buffer_write(void *userData, void const *data, usize size) {
		static_cast<StringBuffer*>(userData)->write(data, size);
	}

	void sb_buffer_resize(void *userData, usize size) {
		static_cast<StringBuffer*>(userData)->resize(size);
	}

}

	String to_str(Allocator *allocator, char v, FmtKind, i32) {
		auto str = allocate<char>(allocator, 1);
		str[0] = v;
		return String{ str, 1 };
	}

	String to_str(Allocator *allocator, u32 v, FmtKind fmtKind, i32 precision) {
		return to_str(allocator, static_cast<u64>(v), fmtKind, precision);
	}

	String to_str(Allocator *allocator, u64 v, FmtKind fmtKind, i32 precision) {
		auto base = choose_base(fmtKind);
		auto letter = fmtKind == FmtKind::HEX_CAP ? 'A' : 'a';
		auto str = allocate<char>(allocator, 66 + precision);
		i64 idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = static_cast<char>(c > 9 ? letter + c - 10 : '0' + c);
			v /= base;
		} while (v > 0);

		for (; idx < precision; ++idx)
			str[idx] = '0';

		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

#ifdef USIZE_OVERRIDE_NEEDED
	String to_str(Allocator *allocator, usize v, FmtKind fmtKind, i32 precision) {
		return to_str(allocator, static_cast<u64>(v), fmtKind, precision);
	}
#endif

	String to_str(Allocator *allocator, i32 v, FmtKind fmtKind, i32 precision) {
		return to_str(allocator, static_cast<i64>(v), fmtKind, precision);
	}

	String to_str(Allocator *allocator, i64 v, FmtKind fmtKind, i32 precision) {
		auto base = choose_base(fmtKind);
		auto letter = fmtKind == FmtKind::HEX_CAP ? 'A' : 'a';
		auto str = allocate<char>(allocator, 66);
		bool neg = false;
		if (v < 0) {
			neg = true;
			v = -v;
		}
		i64 idx = 0;
		do {
			auto c = static_cast<char>(v % base);
			str[idx++] = static_cast<char>(c > 9 ? letter + c - 10 : '0' + c);
			v /= base;
		} while (v > 0);

		for (; idx < precision; ++idx)
			str[idx] = '0';

		if (neg) {
			str[idx++] = '-';
		}

		Slice<char> string{ str, idx };
		reverse(string);
		return string;
	}

	String to_str(Allocator *allocator, f32 v, FmtKind fmtKind, i32 precision) {
		constexpr usize bufSize = 32;
		auto str = make<char>(allocator, bufSize);
		snprintf(
				str, bufSize, choose_snprintf_float_fmt_string(fmtKind), precision, static_cast<f64>(v));
		return str;
	}

	String to_str(Allocator *allocator, f64 v, FmtKind fmtKind, i32 precision) {
		constexpr usize bufSize = 32;
		auto str = make<char>(allocator, bufSize);
		snprintf(
				str, bufSize, choose_snprintf_float_fmt_string(fmtKind), precision, v);
		return str;
	}

	String to_str(Allocator *allocator, char const *v, FmtKind fmtKind, i32) {
		return to_str(allocator, String{ v }, fmtKind);
	}

	String to_str(Allocator *allocator, unsigned char const *v, FmtKind fmtKind, i32) {
		return to_str(allocator, String{ reinterpret_cast<char const *>(v) }, fmtKind);
	}

	String to_str(Allocator *allocator, String str, FmtKind fmtKind, i32) {
		switch (fmtKind) {
		case FmtKind::LOWER:
			{
				auto nstr = allocate<char>(allocator, str.count);
				for (i64 i = 0; i < str.count; ++i)
					nstr[i] = static_cast<char>(tolower(str[i]));
				return String{ nstr, str.count };
			}
			break;
		case FmtKind::UPPER:
			{
				auto nstr = allocate<char>(allocator, str.count);
				for (i64 i = 0; i < str.count; ++i)
					nstr[i] = static_cast<char>(toupper(str[i]));
				return String{ nstr, str.count };
			}
			break;
		case FmtKind::DEFAULT:
		default:
			return str;
			break;
		}
	}

	i64 from_str(char *v, String str) {
		if (!str.count)
			return -1;
		*v = str[0];
		return 1;
	}

	i64 from_str(u8 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<u8>(strtoul(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(u16 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<u16>(strtoul(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(u32 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<u32>(strtoul(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(u64 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = strtoull(cstr, &end, base);

		return end - cstr;
	}

	i64 from_str(i8 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<i8>(strtol(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(i16 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<i16>(strtol(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(i32 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = static_cast<i32>(strtol(cstr, &end, base));

		return end - cstr;
	}

	i64 from_str(i64 *v, String str, i32 base) {
		TMP_ALLOC(66);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 66));
		char *end;
		*v = strtoll(cstr, &end, base);

		return end - cstr;
	}

	i64 from_str(f32 *v, String str) {
		TMP_ALLOC(32);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 32));
		char *end;
		*v = strtof(cstr, &end);

		return end - cstr;
	}

	i64 from_str(f64 *v, String str) {
		TMP_ALLOC(32);

		auto cstr = as_c_str(&tmpAlloc, sub_slice(slc(str), 0, 32));
		char *end;
		*v = strtod(cstr, &end);

		return end - cstr;
	}

	i64 from_str(String *v, String str) {
		*v = str;
		return str.count;
	}

	void IBuffer::write(void const *data, usize size) {
		writeFn(userData, data, size);
	}

	void IBuffer::resize(usize size) {
		resizeFn(userData, size);
	}

	void FileBuffer::write(void const *data, usize size) {
		fwrite(data, 1, size, file);
		fflush(file);
	}

	void StringBuffer::write(void const *data, usize size) {
		assert(size <= buffer->count - pos);
		memcpy(buffer->data + pos, data, size);
		pos += size;
	}

	void StringBuffer::resize(usize size) {
		size += pos;
		auto ndata = make<char>(allocator, size);
		if (buffer->data) {
			memcpy(ndata, buffer->data, buffer->count);
			deallocate(allocator, buffer->data, buffer->count);
		}
		buffer->data = ndata;
		buffer->count = size;
	}

	IBuffer StringBuffer::get_buffer_interface() {
		return { this, sb_buffer_write, sb_buffer_resize };
	}

	void SliceBuffer::write(void const *data, usize size) {
		assert(buffer->count + static_cast<i64>(size) <= capacity);
		memcpy(buffer->data + buffer->count, data, size);
		buffer->count += size;
	}

	void ArrayBuffer::write(void const *data, usize size) {
		assert(*count + static_cast<i64>(size) <= capacity);
		memcpy(buffer + *count, data, size);
		*count += size;
	}

}

