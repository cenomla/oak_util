#pragma once

#include <cinttypes>
#include <cstddef>
#include <cassert>

#include <type_traits>
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

namespace oak {

	struct MemoryArena;

	enum class _reflect(oak::catagory::none) Result {
		SUCCESS,
		INVALID_ARGS,
		OUT_OF_MEMORY,
		FAILED_IO,
		FILE_NOT_FOUND,
	};

	struct Allocator {
		void *state = nullptr;
		void *(*allocFn)(void *self, u64 size, u64 alignment) = nullptr;
		void (*freeFn)(void *self, void *ptr, u64 size) = nullptr;

		void* allocate(u64 size, u64 alignment) {
			if (allocFn) {
				return (*allocFn)(state, size, alignment);
			} else {
				return nullptr;
			}
		}

		void deallocate(void *ptr, u64 size) {
			if (freeFn) {
				(*freeFn)(state, ptr, size);
			}
		}
	};

	template<typename type>
	struct Slice {
		type *data = nullptr;
		i64 count = 0;

		constexpr Slice() noexcept = default;
		constexpr Slice(type *data_, i64 count_) noexcept
			: data{ data_ }, count{ count_ } {}

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

		constexpr type& operator[](i64 index) noexcept {
			return data[index];
		}

		constexpr type const& operator[](i64 index) const noexcept {
			return data[index];
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

	constexpr i64 c_str_len(char const *str) noexcept {
		i64 count = 0;
		while (*str) {
			++str;
			++count;
		}
		return count;
	}

	struct String : Slice<char> {
		using Slice::Slice;

		constexpr String(Slice<char> const slice) noexcept : Slice{ slice } {}
		constexpr String(char const* str) noexcept : Slice{ const_cast<char*>(str), c_str_len(str) } {}

		constexpr String& operator=(Slice<char> const slice) noexcept {
			data = slice.data;
			count = slice.count;
			return *this;
		}

		constexpr operator Slice<char>() const noexcept {
			return { data, count };
		}

	};

	constexpr bool operator==(String const& lhs, char const *rhs) noexcept {
		return lhs == String{ rhs };
	}

	constexpr bool operator!=(String const& lhs, char const *rhs) noexcept {
		return !(lhs == String{ rhs });
	}

	bool is_c_str(String const str);
	const char* as_c_str(String const str);
	String copy_str(MemoryArena *arena, String const str);

	template<typename T>
	struct HashFn {
		constexpr u64 operator()(T const& value) const noexcept {
			if constexpr (std::is_integral_v<T>) {
				return static_cast<u64>(value);
			} else {
				static_assert("hash not supported");
				return 0;
			}
		}
	};

	template<typename T, typename U>
	struct CmpFn {
		constexpr i64 operator()(T const& lhs, U const& rhs) const noexcept {
			if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
				return rhs - lhs;
			} else {
				static_assert("compare not supported");
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
			i64 const count = lhs.count < rhs.count ? lhs.count : rhs.count;
			for (i64 i = 0; i < count; ++i) {
				if (lhs[i] != rhs[i]) {
					return lhs[i] - rhs[i];
				}
			}
			return 0;
		}
	};


}

