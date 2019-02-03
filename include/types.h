#pragma once

#include <cstddef>
#include <cinttypes>
#include <osig_defs.h>

#define ssizeof(x) static_cast<int64_t>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<int64_t>(array_count(x))

namespace oak {

	enum class _reflect(oak::catagory::none) Result {
		SUCCESS,
		INVALID_ARGS,
		FILE_NOT_FOUND,
		FAILED_IO,
		OUT_OF_MEMORY,
	};

}
