#pragma once

#include <cinttypes>
#include <cstring>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace oak {

	inline int clz(uint64_t value) {
#ifdef _MSC_VER
		return __lzcnt64(value);
#else
		return __builtin_clzll(value);
#endif
	}

	inline int ctz(uint64_t value) {
#ifdef _MSC_VER
		unsigned long index = 0;
		_BitScanForward64(&index, value);
		return index;
#else
		return __builtin_ctzll(value);
#endif
	}

	inline int bit_count(uint64_t value) {
#ifdef _MSC_VER
		return __popcnt64(value);
#else
		return __builtin_popcountll(value);
#endif
	}

	inline uint64_t rotate_left(uint64_t const value, int32_t const amount) noexcept {
		return (value << amount) | (value >> (64 - amount));
	}

	inline uint64_t rotate_right(uint64_t const value, int32_t const amount) noexcept {
		return (value >> amount) | (value << (64 - amount));
	}

	inline uint64_t next_pow2(uint64_t value) {
		return 1ull << (64 - clz(value));
	}

	inline int64_t next_pow2(int64_t value) {
		if (value < 0) {
			return -static_cast<int64_t>(next_pow2(static_cast<uint64_t>(-value)));
		}
		return next_pow2(static_cast<uint64_t>(value));
	}

	inline uint64_t ensure_pow2(uint64_t value) {
		return ctz(value) + clz(value) + 1 == 64 ? value : next_pow2(value);
	}

	inline int64_t ensure_pow2(int64_t value) {
		if (value < 0) {
			return -static_cast<int64_t>(ensure_pow2(static_cast<uint64_t>(-value)));
		}
		return static_cast<int64_t>(ensure_pow2(static_cast<uint64_t>(value)));
	}

	inline int32_t ensure_pow2(int32_t value) {
		if (value < 0) {
			return -static_cast<int32_t>(ensure_pow2(static_cast<uint64_t>(-value)));
		}
		return static_cast<int32_t>(ensure_pow2(static_cast<uint64_t>(value)));
	}

	inline uint64_t blog2(uint64_t value) {
		return 63ull - clz(value);
	}

	template<typename T>
	constexpr void set_bit(T& value, int32_t n) noexcept {
		value |= (T{1} << n);
	}

	template<typename T>
	constexpr bool get_bit(T& value, int32_t n) noexcept {
		return value & (T{1} << n);
	}

	template<typename T>
	constexpr void clear_bit(T& value, int32_t n) noexcept {
		value &= ~(T{1} << n);
	}

	template<typename T>
	constexpr void toggle_bit(T& value, int32_t n) noexcept {
		value ^= (T{1} << n);
	}

	template<typename T>
	constexpr void change_bit(T& value, int32_t n, bool set) noexcept {
		auto x = -static_cast<T>(set);
		value ^= (-x ^ value) & (T{1} << n);
	}

	template<typename T, typename U>
	constexpr T bit_cast(U const& u) noexcept {
		static_assert(sizeof(U) <= sizeof(T));
		T result{};
		std::memcpy(&result, &u, sizeof(U));
		return result;
	}

}

