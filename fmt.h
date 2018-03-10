#pragma once

#include <cinttypes>
#include <cstddef>
#include <utility>

#include "string.h"

namespace oak::detail {

	size_t to_str_size(char v);
	size_t to_str_size(uint32_t v);
	size_t to_str_size(uint64_t v);
	size_t to_str_size(int32_t v);
	size_t to_str_size(int64_t v);
	size_t to_str_size(float v);
	size_t to_str_size(String v);

	String to_str(char v);
	String to_str(uint32_t v);
	String to_str(uint64_t v);
	String to_str(int32_t v);
	String to_str(int64_t v);
	String to_str(float v);
	String to_str(String str);

	template<typename Buffer>
	void print_fmt_impl(Buffer&& buffer, String fmtStr, size_t start) {
		auto str = substr(fmtStr, start);
		buffer.write(str.data, str.size);
	}
	
	template<typename Buffer, typename TArg, typename... TArgs>
	void print_fmt_impl(Buffer&& buffer, String fmtStr, size_t start, TArg&& arg, TArgs&&... args) {
		//find each instance of %
		auto pos = fmtStr.find('%', start);
		auto str = substr(fmtStr, start, pos);
		if (pos < fmtStr.size - 1 &&
				fmtStr[pos + 1] == '%') {
			buffer.write("%", 1);
		} else if (pos == decltype(fmtStr)::npos) {
			buffer.write(str.data, str.size);
		} else {
			auto argStr = detail::to_str(arg);
			buffer.write(str.data, str.size);
			buffer.write(argStr.data, argStr.size);
			print_fmt_impl(buffer, fmtStr, pos + 1, std::forward<TArgs>(args)...);
		}
	}
	
}

namespace oak {

	struct Stdout {
		void write(const void *data, size_t size);
	};

	template<typename T>
	struct HasResizeMethod {
		template<typename U, void (U::*)(size_t)> struct SFINAE {};
		template<typename U> static char test(SFINAE<U, &U::resize>*);
		template<typename U> static int test(...);
		static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	struct ArrayBuffer {
		void write(const void *data, size_t size);
		void resize(size_t size);

		Array<char> *buffer = nullptr;
		size_t pos = 0;
	};

	template<typename Buffer, typename... TArgs>
	void print_fmt(Buffer&& buffer, String fmtStr, TArgs&&... args) {
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
			totalSize += fmtStr.size - sizeof...(args);
			buffer.resize(totalSize);
		}
		detail::print_fmt_impl(std::forward<Buffer>(buffer), fmtStr, 0, std::forward<TArgs>(args)...);
	}

}

