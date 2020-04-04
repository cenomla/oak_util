#pragma once

#include <utility>
#include <cstdio>

#include "types.h"
#include "memory.h"
#include "algorithm.h"

namespace oak {
	enum class FmtKind {
		DEFAULT,
		BIN,
		OCT,
		DEC,
		HEX,
		EXP,
	};

	struct FmtSpec {
		FmtKind kind;
		i64 start, end;
	};
}

namespace oak::detail {

	template<typename Buffer>
	void fmt_impl(Buffer&& buffer, String const fmtStr, String const *const strings, FmtSpec const *const specs, i64 const count) {
		i64 pos = 0;
		for (i32 i = 0; i < count; ++i) {
			if (auto const slice = sub_slice(fmtStr, pos, specs[i].start); slice.count > 0) {
				buffer.write(slice.data, slice.count);
			}
			buffer.write(strings[i].data, strings[i].count);
			pos = specs[i].end;
		}
		if (auto const slice = sub_slice(fmtStr, pos); slice.count > 0) {
			buffer.write(slice.data, slice.count);
		}
	}

	inline void fmt_get_spec(String const fmtStr, FmtSpec *const specs, i64 const specCount) {
		i64 idx = 0, start = 0;
		// Fill the specs array with fmt specifiers based off of the %<FMT> syntax
		while (idx < specCount && start < fmtStr.count) {
			auto pos = find(fmtStr, '%', start);
			if (pos != -1 && pos + 1 < fmtStr.count) {
				switch (fmtStr[pos + 1]) {
					case 'g': specs[idx++] = { FmtKind::DEFAULT, pos, pos + 2 }; break;
					case 'b': specs[idx++] = { FmtKind::BIN, pos, pos + 2 }; break;
					case 'o': specs[idx++] = { FmtKind::OCT, pos, pos + 2 }; break;
					case 'd': specs[idx++] = { FmtKind::DEC, pos, pos + 2 }; break;
					case 'x': specs[idx++] = { FmtKind::HEX, pos, pos + 2 }; break;
					case 'e': specs[idx++] = { FmtKind::EXP, pos, pos + 2 }; break;
					default:
						break;
				}
				start = pos + 2;
			} else {
				start = fmtStr.count;
			}
		}

	}

	template<typename... TArgs, usize... Is>
	void fmt_get_strings(
			Allocator *allocator,
			String *const strings,
			FmtSpec *const fmtSpecs,
			std::index_sequence<Is...>,
			TArgs&&... args) {
		((strings[Is] = to_str(allocator, std::forward<TArgs>(args), fmtSpecs[Is].kind)), ...);
	}

}

namespace oak {

	String to_str(Allocator *allocator, char v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, u32 v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, u64 v, FmtKind = FmtKind::DEFAULT);
#ifdef USIZE_OVERRIDE_NEEDED
	String to_str(Allocator *allocator, usize v, FmtKind = FmtKind::DEFAULT);
#endif
	String to_str(Allocator *allocator, i32 v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, i64 v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, f32 v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, f64 v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, char const *v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, unsigned char const *v, FmtKind = FmtKind::DEFAULT);
	String to_str(Allocator *allocator, String str, FmtKind = FmtKind::DEFAULT);

	template<typename T>
	struct HasResizeMethod {
		template<typename U, void (U::*)(u64)> struct SFINAE {};
		template<typename U> static char test(SFINAE<U, &U::resize>*);
		template<typename U> static int test(...);
		static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	struct FileBuffer {
		void write(void const *data, usize size);

		FILE *file = nullptr;
	};

	struct StringBuffer {
		void write(void const *data, usize size);
		void resize(usize size);

		Allocator *allocator = nullptr;
		Slice<char> *buffer = nullptr;
		u64 pos = 0;
	};

	struct SliceBuffer {
		void write(void const *data, usize size);

		Slice<char> *buffer = nullptr;
		u64 capacity = 0;
	};

	template<typename Buffer, typename... TArgs>
	void buffer_fmt(Buffer&& buffer, String const fmtStr, TArgs&&... args) {
		constexpr auto hasResize = HasResizeMethod<std::decay_t<Buffer>>::value;
		if constexpr (sizeof...(args) > 0) {
			// Since arrays of size zero arent supported just add one to the size of args and keep an empty string at the end
			FmtSpec fmtSpecs[sizeof...(args)];
			String argStrings[sizeof...(args)];
			detail::fmt_get_spec(fmtStr, fmtSpecs, sizeof...(args));
			detail::fmt_get_strings(
					&temporaryMemory,
					argStrings,
					fmtSpecs,
					std::make_index_sequence<sizeof...(args)>{},
					std::forward<TArgs>(args)...);

			// If we have control over the buffer size make sure it is of valid size
			if constexpr(hasResize) {
				u64 totalSize = fmtStr.count;
				for (auto str : argStrings) {
					totalSize += str.count;
				}
				totalSize -= sizeof...(args) * 2;
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
	void print_fmt(String const fmtStr, TArgs&&... args) {
		static SpinLock printLock;
		printLock.lock();
		buffer_fmt(FileBuffer{ stdout }, fmtStr, std::forward<TArgs>(args)...);
		printLock.unlock();
	}

	template<typename... TArgs>
	String fmt(Allocator *const allocator, String fmtStr, TArgs&&... args) {
		Slice<char> string;
		buffer_fmt(StringBuffer{ allocator, &string }, fmtStr, std::forward<TArgs>(args)...);
		return string;
	}

}

