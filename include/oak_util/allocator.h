#pragma once

#include <cstdlib>

namespace oak {

	namespace detail {

		inline void std_free_wrapper(void *ptr, size_t) {
			std::free(ptr);
		}

	}

	inline void* (*alloc)(size_t size) = std::malloc;
	inline void (*free)(void *ptr, size_t size) = detail::std_free_wrapper;

}

