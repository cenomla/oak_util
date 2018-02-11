#pragma once

#include "string.h"
#include "array.h"
#include "allocator.h"

namespace oak {

	struct StringBuilder {
		Array<char> string;

		StringBuilder(IAllocator *allocator) : string{ allocator } {};

		void push(String str);
		void push(float v);
		void push(int32_t v);
		void push(int64_t v);
		void push(uint32_t v);
		void push(uint64_t v);
	};
}
