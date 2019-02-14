#pragma once

#include <utility>
#include <cstdio>

#include "memory.h"
#include "algorithm.h"
#include "string.h"

namespace oak::detail {

	String to_str(char v);
	String to_str(uint32_t v);
	String to_str(uint64_t v);
	String to_str(int32_t v);
	String to_str(int64_t v);
	String to_str(float v);
	String to_str(double v);
	String to_str(const char *v);
	String to_str(const unsigned char *v);
	String to_str(String str);

	template<typename Buffer>
	void print_fmt_impl(Buffer&& buffer, String fmtStr, String const *strings, int64_t stringCount) {
		int64_t idx = 0;
		int64_t start = 0;
		// Write each substr replacing % with the argument string
		while (start < fmtStr.count) {
			// Find each instance of %
			auto pos = find(fmtStr, '%', start);
			auto str = sub_slice(fmtStr, start, pos);
			// %% means just print the %
			if (0 < pos && pos + 1 < fmtStr.count && fmtStr[pos + 1] == '%') {
				buffer.write("%", 1);
				// Skip %%
				start = pos + 2;
			} else {
				// Write the slice of the format string to the buffer
				buffer.write(str.data, str.count);
				// If we found a percent then write then replacement string
				if (pos != -1) {
					if (idx < stringCount) {
						buffer.write(strings[idx].data, strings[idx].count);
						++idx;
					}
					// Skip %
					start = pos + 1;
				} else {
					start += str.count;
				}
			}
		}
	}

}

namespace oak {

	template<typename T>
	struct HasResizeMethod {
		template<typename U, void (U::*)(size_t)> struct SFINAE {};
		template<typename U> static char test(SFINAE<U, &U::resize>*);
		template<typename U> static int test(...);
		static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	struct FileBuffer {
		void write(void const *data, size_t size);

		FILE *file = nullptr;
	};

	struct StringBuffer {
		void write(void const *data, size_t size);
		void resize(size_t size);

		Allocator *allocator = nullptr;
		String *buffer = nullptr;
		size_t pos = 0;
	};

	template<typename Buffer, typename... TArgs>
	void buffer_fmt(Buffer&& buffer, String fmtStr, TArgs&&... args) {
		// Since arrays of size zero arent supported just add one to the size of args and keep an empty string at the end
		String const argStrings[] = {
			detail::to_str(std::forward<TArgs>(args))..., "",
		};
		// If we have control over the buffer size make sure it is of valid size
		if constexpr(HasResizeMethod<std::decay_t<Buffer>>::value) {
			size_t totalSize = 0;
			for (auto str : argStrings) {
				totalSize += str.count;
			}
			totalSize += fmtStr.count - sizeof...(args);
			buffer.resize(totalSize);
		}
		detail::print_fmt_impl(std::forward<Buffer>(buffer), fmtStr, argStrings, sizeof...(args));
	}

	template<typename... TArgs>
	void print_fmt(String fmtStr, TArgs&&... args) {
		buffer_fmt(FileBuffer{ stdout }, fmtStr, std::forward<TArgs>(args)...);
	}

	template<typename... TArgs>
	String fmt(MemoryArena *arena, String fmtStr, TArgs&&... args) {
		String string;
		buffer_fmt(StringBuffer{ arena, &string }, fmtStr, std::forward<TArgs>(args)...);
		return string;
	}

}

