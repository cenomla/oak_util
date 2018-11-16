#pragma once

#include <cinttypes>

namespace oak {

	constexpr int clz(uint64_t value) {
		return __builtin_clzll(value);
	}

	constexpr int ctz(uint64_t value) {
		return __builtin_ctzll(value);
	}

	constexpr uint64_t next_pow2(uint64_t value) {
		return 1ull << (64 - clz(value));
	}

	constexpr int64_t next_pow2(int64_t value) {
		if (value < 0) {
			return -static_cast<int64_t>(next_pow2(static_cast<uint64_t>(-value)));
		}
		return next_pow2(static_cast<uint64_t>(value));
	}

	constexpr uint64_t ensure_pow2(uint64_t value) {
		return ctz(value) + clz(value) + 1 == 64 ? value : next_pow2(value);
	}

	constexpr int64_t ensure_pow2(int64_t value) {
		if (value < 0) {
			return -static_cast<int64_t>(ensure_pow2(static_cast<uint64_t>(-value)));
		}
		return static_cast<int64_t>(ensure_pow2(static_cast<uint64_t>(value)));
	}

	constexpr int32_t ensure_pow2(int32_t value) {
		if (value < 0) {
			return -static_cast<int32_t>(ensure_pow2(static_cast<uint64_t>(-value)));
		}
		return static_cast<int32_t>(ensure_pow2(static_cast<uint64_t>(value)));
	}

	constexpr uint64_t blog2(uint64_t value) {
		return 63ull - clz(value);
	}

}

