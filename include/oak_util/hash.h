#pragma once

#include "types.h"
#include "bit.h"

namespace oak {

	constexpr u64 hash_int(u64 v) noexcept {
		// Taken from the stack overflow article: https://stackoverflow.com/a/12996028
		v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9;
		v = (v ^ (v >> 27)) * 0x94d049bb133111eb;
		v = v ^ (v >> 31);
		return v;
	}

	constexpr u64 hash_float(f32 v) noexcept {
		return hash_int(bit_cast<u32>(v));
	}

	constexpr u64 hash_combine(u64 a, u64 b) noexcept {
		// Combine the two hash values using a bunch of random large primes
		return 262147 + a * 131101 + b * 65599;
	}

	template<typename T>
	struct HashFn {
		constexpr u64 operator()(T const& value) const noexcept {
			static_assert("hash not supported" && (std::is_integral_v<T> || std::is_pointer_v<T>));
			if constexpr (std::is_integral_v<T>) {
				return hash_int(static_cast<u64>(value));
			} else if constexpr (std::is_pointer_v<T>) {
				return hash_int(reinterpret_cast<u64>(value));
			} else {
				return 0;
			}
		}
	};

	template<typename T, typename U>
	struct CmpFn {
		constexpr i64 operator()(T const& lhs, U const& rhs) const noexcept {
			static_assert("compare not supported" &&
					((std::is_integral_v<T> && std::is_integral_v<U>)
					 || (std::is_pointer_v<T> && std::is_pointer_v<U>)));
			if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
				return lhs - rhs;
			} else if constexpr (std::is_pointer_v<T> && std::is_pointer_v<U>) {
				return reinterpret_cast<i64>(lhs) - reinterpret_cast<i64>(rhs);
			} else {
				return 0;
			}
		}
	};

	template<>
	struct HashFn<String> {
		constexpr u64 operator()(String const& str) const noexcept {
			u64 hash = 0;

			for (i64 i = 0; i < str.count; ++i) {
				hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
			}

			return hash + 1;
		}
	};

	template<>
	struct CmpFn<String, String> {
		constexpr i64 operator()(String const& lhs, String const& rhs) const noexcept {
			auto count = lhs.count;
			if (rhs.count < count)
				count = rhs.count;

			for (i64 i = 0; i < count; ++i) {
				if (lhs[i] != rhs[i]) {
					return lhs[i] - rhs[i];
				}
			}

			if (lhs.count > rhs.count)
				return lhs[rhs.count];

			if (rhs.count > lhs.count)
				return -rhs[lhs.count];

			return 0;
		}
	};

}
