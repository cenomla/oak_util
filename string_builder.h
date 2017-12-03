#pragma once

#include "string.h"
#include "array.h"
#include "allocator.h"

namespace oak {

	struct StringBuilder {
		Array<char> string{ &listAlloc };

		void push(String str);
	};
}
