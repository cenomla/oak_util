#pragma once

#include <utility>
#include <cstdio>

#include "types.h"
#include "atomic.h"
#include "memory.h"
#include "algorithm.h"

namespace oak {
	enum class FmtKind {
		DEFAULT,
		BIN,
		OCT,
		DEC,
		HEX,
		HEX_CAP,
		EXP,
		LOWER,
		UPPER,
		INVALID,
	};

	struct FmtSpec {
		FmtKind kind;
		i64 start, end;
		i32 precision;
	};
}

namespace oak::detail {

	template<typename Buffer>
	void fmt_impl(Buffer&& buffer, String fmtStr, String const *strings, FmtSpec const *specs, i64 count) {
		i64 pos = 0;
		for (i32 i = 0; i < count; ++i) {
			if (auto const slice = sub_slice(slc(fmtStr), pos, specs[i].start); slice.count > 0) {
				buffer.write(slice.data, slice.count);
			}
			buffer.write(strings[i].data, strings[i].count);
			pos = specs[i].end;
		}
		if (auto const slice = sub_slice(slc(fmtStr), pos); slice.count > 0) {
			buffer.write(slice.data, slice.count);
		}
	}

	constexpr i32 parse_fmt_precision(String fmtStr, i64 *pos) {
		i32 precision = 0;
		++*pos;
		for (; *pos < fmtStr.count; ++*pos) {
			switch (fmtStr[*pos]) {
			case '0': precision *= 10; precision += 0; break;
			case '1': precision *= 10; precision += 1; break;
			case '2': precision *= 10; precision += 2; break;
			case '3': precision *= 10; precision += 3; break;
			case '4': precision *= 10; precision += 4; break;
			case '5': precision *= 10; precision += 5; break;
			case '6': precision *= 10; precision += 6; break;
			case '7': precision *= 10; precision += 7; break;
			case '8': precision *= 10; precision += 8; break;
			case '9': precision *= 10; precision += 9; break;
			default: --*pos; return precision;
			}
		}
		return precision;
	}

	constexpr bool parse_fmt_spec(FmtSpec *fmtSpec, String fmtStr, i64 *pos) {
		i64 start = *pos;
		FmtKind fmtKind = FmtKind::INVALID;
		i32 precision = -1;

		++*pos;
		for (; *pos < fmtStr.count && fmtKind == FmtKind::INVALID; ++*pos) {
			switch (fmtStr[*pos]) {
			case 'g': fmtKind = FmtKind::DEFAULT; break;
			case 'b': fmtKind = FmtKind::BIN; break;
			case 'o': fmtKind = FmtKind::OCT; break;
			case 'd': fmtKind = FmtKind::DEC; break;
			case 'x': fmtKind = FmtKind::HEX; break;
			case 'X': fmtKind = FmtKind::HEX_CAP; break;
			case 'e': fmtKind = FmtKind::EXP; break;
			case 'c': fmtKind = FmtKind::LOWER; break;
			case 'C': fmtKind = FmtKind::UPPER; break;
			case '.': precision = parse_fmt_precision(fmtStr, pos); break;
			default: return false;
			}
		}

		if (fmtKind == FmtKind::INVALID)
			return false;

		*fmtSpec = { fmtKind, start, *pos, precision };
		return true;
	}

	constexpr void fmt_get_spec(String fmtStr, FmtSpec *specs, i64 specCount) {
		i64 idx = 0, start = 0;
		// Fill the specs array with fmt specifiers based off of the %<FMT> syntax
		while (idx < specCount && start < fmtStr.count) {
			auto pos = find(slc(fmtStr), '%', start);
			if (pos != -1) {
				if (FmtSpec spec{}; parse_fmt_spec(&spec, fmtStr, &pos))
					specs[idx++] = spec;
				start = pos;
			} else {
				start = fmtStr.count;
			}
		}

	}

	template<typename... TArgs, usize... Is>
	constexpr void fmt_get_strings(
			Allocator *allocator,
			String *strings,
			FmtSpec *fmtSpecs,
			std::index_sequence<Is...>,
			TArgs&&... args) {
		((strings[Is] = to_str(
							   allocator,
							   std::forward<TArgs>(args),
							   fmtSpecs[Is].kind,
							   fmtSpecs[Is].precision)), ...);
	}

}

namespace oak {

	OAK_UTIL_API String to_str(
			Allocator *allocator, char v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, u32 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, u64 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
#ifdef USIZE_OVERRIDE_NEEDED
	OAK_UTIL_API String to_str(
			Allocator *allocator, usize v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
#endif
	OAK_UTIL_API String to_str(
			Allocator *allocator, i32 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, i64 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, f32 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, f64 v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, char const *v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, unsigned char const *v, FmtKind = FmtKind::DEFAULT, i32 precision = -1);
	OAK_UTIL_API String to_str(
			Allocator *allocator, String str, FmtKind = FmtKind::DEFAULT, i32 precision = -1);

	OAK_UTIL_API i32 from_str(char *v, String str);
	OAK_UTIL_API i32 from_str(u8 *v, String str);
	OAK_UTIL_API i32 from_str(u16 *v, String str);
	OAK_UTIL_API i32 from_str(u32 *v, String str);
	OAK_UTIL_API i32 from_str(u64 *v, String str);
	OAK_UTIL_API i32 from_str(i8 *v, String str);
	OAK_UTIL_API i32 from_str(i16 *v, String str);
	OAK_UTIL_API i32 from_str(i32 *v, String str);
	OAK_UTIL_API i32 from_str(i64 *v, String str);
	OAK_UTIL_API i32 from_str(f32 *v, String str);
	OAK_UTIL_API i32 from_str(f64 *v, String str);
	OAK_UTIL_API i32 from_str(String *v, String str);

	template<typename T>
	struct HasResizeMethod {
		template<typename U, void (U::*)(u64)> struct SFINAE {};
		template<typename U> static char test(SFINAE<U, &U::resize>*);
		template<typename U> static int test(...);
		static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	struct OAK_UTIL_API FileBuffer {
		void write(void const *data, usize size);

		FILE *file = nullptr;
	};

	struct OAK_UTIL_API StringBuffer {
		void write(void const *data, usize size);
		void resize(usize size);

		Allocator *allocator = nullptr;
		Slice<char> *buffer = nullptr;
		u64 pos = 0;
	};

	struct OAK_UTIL_API SliceBuffer {
		void write(void const *data, usize size);

		Slice<char> *buffer = nullptr;
		i64 capacity = 0;
	};

	struct OAK_UTIL_API ArrayBuffer {

		ArrayBuffer() = default;
		template<typename Array>
		ArrayBuffer(Array& array)
			: buffer{ array.data }, count{ &array.count }, capacity{ array.capacity } {}

		void write(void const *data, usize size);

		char *buffer = nullptr;
		i64 *count = nullptr;
		i64 capacity = 0;
	};

	template<typename Buffer, typename... TArgs>
	void buffer_fmt(Buffer&& buffer, String fmtStr, TArgs&&... args) {
		constexpr auto hasResize = HasResizeMethod<std::decay_t<Buffer>>::value;
		if constexpr (sizeof...(args) > 0) {
			FmtSpec fmtSpecs[sizeof...(args)]{};
			String argStrings[sizeof...(args)]{};
			detail::fmt_get_spec(fmtStr, fmtSpecs, sizeof...(args));
			detail::fmt_get_strings(
					temporaryAllocator,
					argStrings,
					fmtSpecs,
					std::make_index_sequence<sizeof...(args)>{},
					std::forward<TArgs>(args)...);

			// If we have control over the buffer size make sure it is of valid size
			if constexpr(hasResize) {
				u64 totalSize = fmtStr.count;
				for (auto fmtSpec : fmtSpecs) {
					totalSize -= (fmtSpec.end - fmtSpec.start);
				}
				for (auto str : argStrings) {
					totalSize += str.count;
				}
				buffer.resize(totalSize);
			}
			detail::fmt_impl(std::forward<Buffer>(buffer), fmtStr, argStrings, fmtSpecs, sizeof...(args));
		} else {
			if constexpr(hasResize)
				buffer.resize(fmtStr.count);

			buffer.write(fmtStr.data, fmtStr.count);
		}
	}

	template<typename... TArgs>
	void print_fmt(String fmtStr, TArgs&&... args) {
		static i32 printLock;
		atomic_lock(&printLock);
		buffer_fmt(FileBuffer{ stdout }, fmtStr, std::forward<TArgs>(args)...);
		atomic_unlock(&printLock);
	}

	template<typename... TArgs>
	[[nodiscard]] String fmt(Allocator *allocator, String fmtStr, TArgs&&... args) {
		Slice<char> string;
		buffer_fmt(StringBuffer{ allocator, &string }, fmtStr, std::forward<TArgs>(args)...);
		return string;
	}

}
