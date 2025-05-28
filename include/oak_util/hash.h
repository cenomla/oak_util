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

	constexpr u64 hash_string(char const *data, i64 count) noexcept {
		// Constexpr murmur hash 64a. This version under gcc O3 generates the same instructions
		// as the optimized version which uses reinterpret_cast.
		// The block read loop gets unrolled into a single mov instruction.
		// Big endian architectures use a single mov instruction paired with a bswap.
		u64 seed = u64{ 0xc70f6907 };
		u64 m = u64{ 0xc6a4a7935bd1e995 };
		u64 r = 47;

		i64 c = count & (~i64{7});
		u64 h = seed ^ (count*m);

		auto end = data + c;

		for (; data != end; data += 8) {
			u64 k = 0;

			for (u64 i = 0; i < 8; ++i)
				k ^= static_cast<u64>(static_cast<u8>(data[i])) << (i*8);

			k *= m;
			k ^= k >> r;
			k *= m;

			h ^= k;
			h *= m;
		}

		if (count & 7) {
			for (u64 i = 0; i < static_cast<u64>(count & 7); ++i)
				h ^= static_cast<u64>(static_cast<u8>(data[i])) << (i*8);
			h *= m;
		}

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	constexpr u64 hash_combine(u64 a, u64 b) noexcept {
		u64 m = u64{ 0xc6a4a7935bd1e995 };
		return (u64{ 0xc70f6907 } ^ (a*m)) ^ (b*m);
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
			return hash_string(str.data, str.count);
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
