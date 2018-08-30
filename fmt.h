#pragma once

#include <cstdio>
#include <utility>

#include "memory.h"
#include "string.h"

namespace oak::detail {

	size_t to_str_size(char v);
	size_t to_str_size(uint32_t v);
	size_t to_str_size(uint64_t v);
	size_t to_str_size(int32_t v);
	size_t to_str_size(int64_t v);
	size_t to_str_size(float v);
	size_t to_str_size(double v);
	size_t to_str_size(const char *v);
	size_t to_str_size(const unsigned char *v);
	size_t to_str_size(String v);

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
	void print_fmt_impl(Buffer&& buffer, String fmtStr, size_t start) {
		auto str = substr(fmtStr, start);
		buffer.write(str.data, str.count);
	}

	template<typename Buffer, typename TArg, typename... TArgs>
	void print_fmt_impl(Buffer&& buffer, String fmtStr, size_t start, TArg&& arg, TArgs&&... args) {
		//find each instance of %
		auto pos = fmtStr.find('%', start);
		auto str = substr(fmtStr, start, pos);
		if (pos < fmtStr.count - 1 &&
				fmtStr[pos + 1] == '%') {
			buffer.write("%", 1);
		} else if (pos == -1) {
			buffer.write(str.data, str.count);
		} else {
			auto argStr = detail::to_str(arg);
			buffer.write(str.data, str.count);
			buffer.write(argStr.data, argStr.count);
			print_fmt_impl(buffer, fmtStr, pos + 1, std::forward<TArgs>(args)...);
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
		void write(const void *data, size_t size);

		FILE *file = nullptr;
	};

	struct StringBuffer {
		void write(const void *data, size_t size);
		void resize(size_t size);

		MemoryArena *arena = nullptr;
		String *buffer = nullptr;
		size_t pos = 0;
	};

	template<typename Buffer, typename... TArgs>
	void buffer_fmt(Buffer&& buffer, String fmtStr, TArgs&&... args) {
		if constexpr(HasResizeMethod<std::decay_t<Buffer>>::value) {
			size_t totalSize = 0;
			if constexpr(sizeof...(args) > 0) {
				const size_t sizes[] = {
					detail::to_str_size(std::forward<TArgs>(args))...,
				};
				for (auto size : sizes) {
					totalSize += size;
				}
			}
			totalSize += fmtStr.count - sizeof...(args);
			buffer.resize(totalSize);
		}
		detail::print_fmt_impl(std::forward<Buffer>(buffer), fmtStr, 0, std::forward<TArgs>(args)...);
	}

	template<typename... TArgs>
	void print_fmt(String fmtStr, TArgs&&... args) {
		buffer_fmt(FileBuffer{ stdout }, fmtStr, std::forward<TArgs>(args)...);
	}

	template<typename... TArgs>
	String fmt(MemoryArena *arena, String fmtStr, TArgs&&... args) {
		String string;
		buffer_fmt(ArrayBuffer{ arena, &string }, fmtStr, std::forward<TArgs>(args)...);
		return string;
	}

}

