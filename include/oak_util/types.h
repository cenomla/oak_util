#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include <type_traits>
#include <tuple>
#include <utility>

#include "defines.h"

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using usize = size_t;
using isize = ptrdiff_t;

using f32 = float;
using f64 = double;

using b8 = bool;
using b32 = uint32_t;

using c8 = char;

static_assert(sizeof(b8) == 1, "b8 type isn't 1 byte");
static_assert(sizeof(usize) == sizeof(isize), "size types aren't the same width");

#define ssizeof(x) static_cast<isize>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<isize>(array_count(x))

namespace oak {

	struct Allocator;

	enum class _reflect(oak::catagory::none) Result {
		SUCCESS,
		INVALID_ARGS,
		OUT_OF_MEMORY,
		FAILED_IO,
		FILE_NOT_FOUND,
	};


	template<typename T>
	struct Slice {

		T *data = nullptr;
		i64 count = 0;

		template<i64 C>
		static constexpr Slice<T> from_array(T const (&array)[C]) {
			return { array, C };
		}

		constexpr T* begin() noexcept {
			return data;
		}

		constexpr T* end() noexcept {
			return data + count;
		}

		constexpr T const* begin() const noexcept {
			return data;
		}

		constexpr T const* end() const noexcept {
			return data + count;
		}

		constexpr T& operator[](i64 const idx) noexcept {
			assert(0 <= idx && idx < count);
			return data[idx];
		}

		constexpr T const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < count);
			return data[idx];
		}

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, count };
		}
	};

	template<typename First>
	Slice(First*, i64) -> Slice<First>;

	template<typename type>
	constexpr bool operator==(Slice<type> const lhs, Slice<type> const rhs) noexcept {
		if (lhs.count != rhs.count)
			return false;
		if (lhs.data == rhs.data)
			return true;

		for (i64 i = 0; i < lhs.count; ++i) {
			if (lhs[i] != rhs[i])
				return false;
		}

		return true;
	}

	template<typename T>
	constexpr bool operator!=(Slice<T> const lhs, Slice<T> const rhs) noexcept {
		return !operator==(lhs, rhs);
	}

	template<typename T, isize N>
	struct _reflect(array) FixedArray {

		using ElemType = T;

		_reflect() static constexpr i64 capacity = N;

		_reflect() T data[N] = {};

		constexpr T* begin() noexcept {
			return data;
		}

		constexpr T* end() noexcept {
			return data + capacity;
		}

		constexpr T const* begin() const noexcept {
			return data;
		}

		constexpr T const* end() const noexcept {
			return data + capacity;
		}

		constexpr T& operator[](i64 const idx) noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr T const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, capacity };
		}

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, capacity };
		}

	};

	template<typename First, typename... Rest>
	struct AllSame {
		static_assert((std::is_same_v<First, Rest> && ...), "All types must be the same");

		using Type = First;
	};

	template<typename First, typename... Rest>
	FixedArray(First, Rest...) -> FixedArray<typename AllSame<First, Rest...>::Type, 1 + sizeof...(Rest)>;

	template<typename T, isize N>
	struct _reflect(array) Array {

		using ElemType = T;

		_reflect() static constexpr i64 capacity = N;

		_reflect() T data[N] = {};
		_reflect() i64 count = 0;

		template<isize C>
		static constexpr Array from(T const (&v)[C]) {
			static_assert(C <= capacity);

			Array result;
			memcpy(result.data, v, C*sizeof(T));
			result.count = C;

			return result;
		}

		template<isize C>
		static constexpr Array from(FixedArray<T, C> const& v) {
			static_assert(C <= capacity);

			Array result;
			memcpy(result.data, v.data, C*sizeof(T));
			result.count = C;

			return result;
		}

		static constexpr Array from_range(T const *data, i64 count) {
			assert(count <= capacity);

			Array result;
			memcpy(result.data, data, count*sizeof(T));
			result.count = count;

			return result;
		}

		constexpr void clear() noexcept {
			count = 0;
		}

		constexpr T* begin() noexcept {
			return data;
		}

		constexpr T* end() noexcept {
			return data + count;
		}

		constexpr T const* begin() const noexcept {
			return data;
		}

		constexpr T const* end() const noexcept {
			return data + count;
		}

		constexpr T& operator[](i64 const idx) noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr T const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, count };
		}

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, count };
		}

	};

	constexpr i64 c_str_len(char const *const str) noexcept {
		if (!str)
			return 0;
		i64 count = 0;
		while (str[count]) {
			++count;
		}
		return count;
	}

	struct _reflect(array) String {

		using ElemType = char const;

		_reflect() char const *data = nullptr;
		_reflect() i64 count = 0;

		constexpr String() noexcept = default;
		constexpr String(char const *data_, i64 count_) noexcept : data{ data_ }, count{ count_ } {}
		constexpr String(char const* str) noexcept : data{ str }, count{ c_str_len(str) } {}

		constexpr String(Slice<char> other) noexcept : data{ other.data }, count{ other.count } {}
		constexpr String(Slice<char const> other) noexcept : data{ other.data }, count{ other.count } {}

		// TODO: Making these constexpr in c++20 should be allowed
		String(Slice<unsigned char> other) noexcept
			: data{ reinterpret_cast<char*>(other.data) }, count{ other.count } {}
		String(Slice<unsigned char const> other) noexcept
			: data{ reinterpret_cast<char const*>(other.data) }, count{ other.count } {}

		constexpr char const* begin() const noexcept {
			return data;
		}

		constexpr char const* end() const noexcept {
			return data + count;
		}

		constexpr char const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < count);
			return data[idx];
		}

		constexpr operator Slice<char const>() const noexcept {
			return Slice<char const>{ data, count };
		}

	};

	constexpr bool operator==(String const lhs, String const rhs) noexcept {
		if (lhs.count != rhs.count)
			return false;
		if (lhs.data == rhs.data)
			return true;

		for (i64 i = 0; i < lhs.count; ++i) {
			if (lhs[i] != rhs[i])
				return false;
		}

		return true;
	}

	constexpr bool operator!=(String const lhs, String const rhs) noexcept {
		return !operator==(lhs, rhs);
	}

	constexpr bool operator==(String const lhs, char const *rhs) noexcept {
		return lhs == String{ rhs };
	}

	constexpr bool operator!=(String const lhs, char const *rhs) noexcept {
		return !(lhs == String{ rhs });
	}

	template<typename T>
	struct ScopeExit {

		T functor;

		constexpr ScopeExit(T &&functor_) noexcept : functor{ std::forward<T>(functor_) } {}

		~ScopeExit() noexcept {
			functor();
		}
	};

#define SCOPE_EXIT(x) oak::ScopeExit MACRO_CAT(_oak_scope_exit_, __LINE__){ [&](){ (x); }}
#define SCOPE_EXIT_BLOCK(x) oak::ScopeExit MACRO_CAT(_oak_scope_exit_, __LINE__){ [&](){ x }}

	template<typename T>
	constexpr auto eni(T val) noexcept {
		if constexpr (std::is_enum_v<T>) {
			return static_cast<std::underlying_type_t<T>>(val);
		} else {
			static_assert("\"enum_int\" must be used with an enum type");
		}
	}

	namespace {

		template<typename ArrayType, typename RawArrayType = std::remove_reference_t<ArrayType>>
		using ArraySliceElem = std::conditional_t<
			std::is_const_v<RawArrayType>, typename RawArrayType::ElemType const, typename RawArrayType::ElemType>;

	}

	template<typename ArrayType, typename E = ArraySliceElem<ArrayType>>
	constexpr Slice<E> slc(ArrayType&& array) noexcept {
		return static_cast<Slice<E>>(array);
	}

	template<typename ArrayType, typename E = ArraySliceElem<ArrayType>>
	constexpr Slice<E const> slc_const(ArrayType&& array) noexcept {
		return static_cast<Slice<E const>>(array);
	}

	template<typename E>
	constexpr Slice<E> slc_value(E& elem) noexcept {
		return { &elem, 1 };
	}
}

/*
namespace oak::detail {

	template<usize I, typename T>
	struct TupleLeaf {
		T value{};

		constexpr auto& get_value(std::integral_constant<usize, I>) { return value; }
		constexpr auto const& get_value(std::integral_constant<usize, I>) const { return value; }
	};

	template<typename... T>
	struct TupleImpl;

	template<usize... Is, typename... Ts>
	struct TupleImpl<std::index_sequence<Is...>, Ts...> : TupleLeaf<Is, Ts>... {

		constexpr TupleImpl() : TupleLeaf<Is, Ts>{}... {}

		constexpr TupleImpl(Ts... args)
			: TupleLeaf<Is, Ts>{ std::forward<Ts>(args) }... {
		}

		constexpr TupleImpl(TupleImpl<std::index_sequence<Is...>, std::decay_t<Ts>...> const& other)
			: TupleLeaf<Is, Ts>{ other.TupleLeaf<Is, std::decay_t<Ts>>::value } ... { }

		constexpr TupleImpl& operator=(TupleImpl<std::index_sequence<Is...>, std::decay_t<Ts>...> const& other) {
			((TupleLeaf<Is, Ts>::value = other.TupleLeaf<Is, std::decay_t<Ts>>::value), ...);
			return *this;
		}

		constexpr TupleImpl(TupleImpl<std::index_sequence<Is...>, std::decay_t<Ts>...>&& other)
			: TupleLeaf<Is, Ts>{ std::move(other.TupleLeaf<Is, std::decay_t<Ts>>::value) } ... { }

		constexpr TupleImpl& operator=(TupleImpl<std::index_sequence<Is...>, std::decay_t<Ts>...>&& other) {
			((TupleLeaf<Is, Ts>::value = std::move(other.TupleLeaf<Is, std::decay_t<Ts>>::value)), ...);
			return *this;
		}

		constexpr TupleImpl(TupleImpl const& other)
			: TupleLeaf<Is, Ts>{ other.TupleLeaf<Is, Ts>::value } ... { }

		constexpr TupleImpl& operator=(TupleImpl const& other) {
			((TupleLeaf<Is, Ts>::value = other.TupleLeaf<Is, Ts>::value), ...);
			return *this;
		}

		constexpr TupleImpl(TupleImpl&& other)
			: TupleLeaf<Is, Ts>{ std::move(other.TupleLeaf<Is, Ts>::value) } ... { }

		constexpr TupleImpl& operator=(TupleImpl&& other) {
			((TupleLeaf<Is, Ts>::value = std::move(other.TupleLeaf<Is, Ts>::value)), ...);
			return *this;
		}

		using TupleLeaf<Is, Ts>::get_value...;

		template<usize I>
		constexpr auto& get() {
			return get_value(std::integral_constant<usize, I>{});
		}

		template<usize I>
		constexpr auto const& get() const {
			return get_value(std::integral_constant<usize, I>{});
		}

	};
}

namespace oak {

	template<typename... Ts>
	using Tuple = detail::TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

	template<typename... Ts>
	auto make_tuple(Ts&&... args) {
		return Tuple<std::decay_t<Ts>...>{ std::forward<Ts>(args)... };
	}

}

namespace std {
    template<usize I, typename... Ts>
    struct tuple_element<I, oak::Tuple<Ts...>> {
        using type = decltype(oak::Tuple<Ts...>{}.template get<I>());
    };

    template<typename... Ts>
    struct tuple_size<oak::Tuple<Ts...>>
        : std::integral_constant<usize, sizeof...(Ts)> { };


	template<usize I, typename... Ts>
	constexpr auto& get(oak::Tuple<Ts...>& tuple) {
		return tuple.template get<I>();
	}

	template<usize I, typename... Ts>
	constexpr auto const& get(oak::Tuple<Ts...>const & tuple) {
		return tuple.template get<I>();
	}
}

*/
