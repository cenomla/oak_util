#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include <type_traits>
#include <tuple>
#include <utility>

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

using b32 = uint32_t;

using byte = unsigned char;

#ifndef NDEBUG
#define OAK_UNREACHABLE(str) assert(str && false)
#else
#if defined(_MSC_VER) && !defined(__clang__)
#define OAK_UNREACHABLE(str) __assume(false)
#else
#define OAK_UNREACHABLE(str) __builtin_unreachable()
#endif
#endif

#ifdef _MSC_VER

#ifdef OAK_UTIL_DYNAMIC_LIB

#ifdef OAK_UTIL_EXPORT_SYMBOLS
#define OAK_UTIL_API __declspec(dllexport)
#else
#define OAK_UTIL_API __declspec(dllimport)
#endif // OAK_UTIL_EXPORT_SYMBOLS

#else

#define OAK_UTIL_API

#endif // OAK_UTIL_DYNAMIC_LIB

#else

#define OAK_UTIL_API __attribute__((visibility("default")))

#endif // _MSC_VER

#if !(defined(__GNUG__) || defined(_MSC_VER))
// Some stdlib implementations treat uint64_t and size_t as different types so we override in that case,
// if we we're to always enable the override then we'd get multiple function definition errors (lol, treat two of the same types as different types but only on some platforms, thanks c++!)
#define USIZE_OVERRIDE_NEEDED
#endif

#define ssizeof(x) static_cast<isize>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<isize>(array_count(x))

#define MACRO_CAT_IMPL(x, y) x##y
#define MACRO_CAT(x, y) MACRO_CAT_IMPL(x, y)

#ifndef __OSIG_REFLECT_MACRO__
#define __OSIG_REFLECT_MACRO__

#ifdef __OSIG__
#define _reflect(...) __attribute__((annotate("reflect;" #__VA_ARGS__)))
#else
#define _reflect(...)
#endif

#endif //__OSIG_REFLECT_MACRO__

#ifdef _MSC_VER
#define OAK_DBG_NO_OPT
#else
#define OAK_DBG_NO_OPT __attribute__((optimize("O0")))
#endif

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

		constexpr Slice() noexcept = default;
		constexpr Slice(T *data_, i64 count_) noexcept : data{ data_ }, count{ count_ } {}
		template<int C>
		constexpr Slice(T (&array)[C]) noexcept : data{ array }, count{ C } {}

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

	template<typename T, usize N>
	struct _reflect(array) FixedArray {

		using ElemType = T;

		_reflect() static constexpr i64 capacity = static_cast<i64>(N);

		_reflect() T data[N]{};

		constexpr FixedArray() noexcept = default;

		template<int C>
		constexpr FixedArray(T (&array)[C]) noexcept {
			static_assert(C <= N);
			memcpy(data, array, capacity * sizeof(T));
		}

		constexpr FixedArray(std::initializer_list<T> list) noexcept {
			auto count = static_cast<i64>(list.size()) < capacity
				? static_cast<i64>(list.size()) : capacity;
			memcpy(data, list.begin(), count * sizeof(T));
		}

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

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, capacity };
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, capacity };
		}
	};

	template<typename T, usize N>
	struct _reflect(array) Array {

		using ElemType = T;

		_reflect() static constexpr i64 capacity = static_cast<i64>(N);

		_reflect() T data[N]{};
		_reflect() i64 count = 0;

		constexpr Array() noexcept = default;

		template<int C>
		constexpr Array(T (&array)[C]) noexcept : count{ C } {
			static_assert(C <= N);
			memcpy(data, array, count * sizeof(T));
		}

		constexpr Array(std::initializer_list<T> list) noexcept
				: count{ static_cast<i64>(list.size()) < capacity ? static_cast<i64>(list.size()) : capacity } {
			memcpy(data, list.begin(), count * sizeof(T));
		}

		template<typename U>
		constexpr Array(Slice<U> slice) noexcept : count{ slice.count < capacity ? slice.count : capacity } {
			memcpy(data, slice.data, count * sizeof(T));
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

		constexpr operator Slice<ElemType const>() const noexcept {
			return Slice<T const>{ data, count };
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, count };
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


	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr Slice<E> slc(ArrayType& array) noexcept {
		return static_cast<Slice<E>>(array);
	}

	template<typename ArrayType, typename E = typename ArrayType::ElemType>
	constexpr Slice<E> slc(ArrayType&& array) noexcept {
		return static_cast<Slice<E>>(array);
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
