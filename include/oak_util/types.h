#pragma once

#include <cinttypes>
#include <cstddef>
#include <cassert>

#include <type_traits>
#include <atomic>
#include <tuple>

#include <osig_defs.h>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using b32 = uint32_t;

#define ssizeof(x) static_cast<i64>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<i64>(array_count(x))

#define MACRO_CAT_IMPL(x, y) x##y
#define MACRO_CAT(x, y) MACRO_CAT_IMPL(x, y)

namespace oak {

	struct Allocator;

	enum class _reflect(oak::catagory::none) Result {
		SUCCESS,
		INVALID_ARGS,
		OUT_OF_MEMORY,
		FAILED_IO,
		FILE_NOT_FOUND,
	};

	template<typename type>
	struct Slice {
		type *data = nullptr;
		i64 count = 0;

		constexpr Slice() noexcept = default;
		constexpr Slice(type *data_, i64 count_) noexcept
			: data{ data_ }, count{ count_ } {}
		template<int C>
		Slice(type const (&array)[C]) noexcept
			: data{ const_cast<type*>(&array[0]) }, count{ C } {}

		constexpr type* begin() noexcept {
			return data;
		}

		constexpr type* end() noexcept {
			return data + count;
		}

		constexpr type const* begin() const noexcept {
			return data;
		}

		constexpr type const* end() const noexcept {
			return data + count;
		}

		constexpr type& operator[](i64 const idx) noexcept {
			return data[idx];
		}

		constexpr type const& operator[](i64 const idx) const noexcept {
			return data[idx];
		}
	};

	template<typename type>
	constexpr bool operator==(Slice<type> const& lhs, Slice<type> const rhs) noexcept {
		if (lhs.count != rhs.count) { return false; }
		if (lhs.data == rhs.data) { return true; }

		for (i64 i = 0; i < lhs.count; ++i) {
			if (lhs[i] != rhs[i]) {
				return false;
			}
		}

		return true;
	}

	template<typename T>
	constexpr bool operator!=(Slice<T> const& lhs, Slice<T> const& rhs) noexcept {
		return !operator==(lhs, rhs);
	}

	constexpr i64 c_str_len(char const *const str) noexcept {
		i64 count = 0;
		while (str[count]) {
			++count;
		}
		return count;
	}

	struct _reflect(oak::catagory::primitive) String : Slice<char const> {
		using Slice<char const>::Slice;

		constexpr String(char const* str) noexcept : Slice{ str, c_str_len(str) } {}

		constexpr String(Slice<char const> const other) noexcept : Slice{ other } {}
		constexpr String(Slice<char> const other) noexcept : Slice{ other.data, other.count } {}

	};

	constexpr bool operator==(String const& lhs, char const *rhs) noexcept {
		return lhs == String{ rhs };
	}

	constexpr bool operator!=(String const& lhs, char const *rhs) noexcept {
		return !(lhs == String{ rhs });
	}

	template<typename T>
	struct HashFn {
		constexpr u64 operator()(T const& value) const noexcept {
			static_assert("hash not supported" && (std::is_integral_v<T> || std::is_pointer_v<T>));
			if constexpr (std::is_integral_v<T>) {
				return static_cast<u64>(value);
			} else if constexpr (std::is_pointer_v<T>) {
				return reinterpret_cast<u64>(value);
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
				return rhs - lhs;
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

			for (i64 i = 0; i < str.count; i++) {
				hash = str.data[i] + (hash << 6) + (hash << 16) - hash;
			}

			return hash + 1;
		}
	};

	template<>
	struct CmpFn<String, String> {
		constexpr i64 operator()(String const& lhs, String const& rhs) const noexcept {
			if (lhs.count > rhs.count) { return lhs[rhs.count]; }
			if (rhs.count > lhs.count) { return rhs[lhs.count]; }

			for (i64 i = 0; i < lhs.count; ++i) {
				if (lhs[i] != rhs[i]) {
					return lhs[i] - rhs[i];
				}
			}

			return 0;
		}
	};

	struct SpinLock {
		std::atomic_flag flag = ATOMIC_FLAG_INIT;

		bool try_lock() noexcept {
			return !flag.test_and_set(std::memory_order_acquire);
		}

		void lock() noexcept {
			while (flag.test_and_set(std::memory_order_acquire));
		}

		void unlock() noexcept {
			flag.clear(std::memory_order_release);
		}

	};


	template<typename T>
	struct ScopeExit {

		T &&functor;

		constexpr ScopeExit(T &&functor_) noexcept : functor{ std::forward<T>(functor_) } {}

		~ScopeExit() noexcept {
			functor();
		}
	};

#define SCOPE_EXIT(x) ScopeExit MACRO_CAT(_oak_scope_exit_, __LINE__){ [](){ (x); }}

}

