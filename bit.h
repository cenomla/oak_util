#pragma once

#include <limits>

namespace oak {

	constexpr size_t clz(size_t value) {
		return __builtin_clzll(value);
	}

	constexpr size_t ctz(size_t value) {
		return __builtin_ctzll(value);
	}

	constexpr size_t msb(size_t value) {
		return std::numeric_limits<size_t>::digits - clz(value);
	}

	constexpr size_t lsb(size_t value) {
		return ctz(value) + 1;
	}

	constexpr size_t npow2(size_t value) {
		return 1lu << msb(value);
	}

	constexpr size_t log2(size_t value) {
		return msb(value) - 1;
	}

}

