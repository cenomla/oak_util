#pragma once

#include "string.h"
#include "array.h"
#include "allocator.h"

namespace oak {

	struct StringBuilder {
		Array<char> string;

		StringBuilder(Allocator *allocator) : string{ allocator } {};

		void push(String str);
	};
}
