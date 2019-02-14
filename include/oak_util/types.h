#pragma once

#include <cinttypes>
#include <cstddef>
#include <cassert>

#include <type_traits>
#include <tuple>

#include <osig_defs.h>
#include "ptr.h"

#define ssizeof(x) static_cast<int64_t>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<int64_t>(array_count(x))

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

using b8 = bool;
using b32 = uint32_t;

namespace oak {

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
	};

	template<typename type>
	struct Slice {
		type *data = nullptr;
		i64 count = 0;

		constexpr type& operator[](i64 index) noexcept {
			return data[index];
		}

		constexpr type const& operator[](i64 index) const noexcept {
			return data[index];
		}
	};

	struct String : Slice<char> {
		using Slice::Slice;
	};

	template<typename... types>
	struct SOA {
		using tuple = std::tuple<types...>;

		void *data = nullptr;
		i64 count = 0;

		constexpr std::tuple<types&...> operator[](i64 const index) noexcept;
	};

}

namespace std {

	template<typename... types>
	struct tuple_size<oak::SOA<types...>> {
		static constexpr size_t value = sizeof...(types);
	};

	template<size_t index, typename... types>
	struct tuple_element<index, oak::SOA<types...>> {
		using type = oak::Slice<typename tuple_element<index, tuple<types...>>::type>;
	};

}

namespace oak {

	template<size_t index, typename... types>
	constexpr typename std::tuple_element<index, SOA<types...>>::type get(SOA<types...>& soa) {
		constexpr size_t sizes[] = { sizeof(types)..., };
		constexpr size_t aligns[] = { alignof(types)..., };

		i64 offset = 0;
		for (size_t i = 0; i < index; ++i) {
			offset = align_int(offset, aligns[i]);
			offset += soa.count * sizes[i];
		}
		offset = align_int(offset, aligns[index]);

		return {
			static_cast<typename std::tuple_element<index, std::tuple<types...>>::type*>(add_ptr(soa.data, offset)),
			soa.count
		};
	}

	namespace detail {

		template<typename tupleT, typename soaT, size_t... ints>
		constexpr tupleT soa_sub_script_impl(soaT& soa, i64 const index, std::index_sequence<ints...>) {
			return { get<ints>(soa)[index]..., };
		}

	}

	template<typename... types>
	constexpr std::tuple<types&...> SOA<types...>::operator[](i64 const index) noexcept {
		using indices = std::make_index_sequence<std::tuple_size<std::tuple<types...>>::value>;
		return detail::soa_sub_script_impl<std::tuple<types&...>>(*this, index, indices{});
	}

}

