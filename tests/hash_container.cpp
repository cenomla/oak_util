#include "oak_util/hash_comp.h"
#include <cstdio>

namespace mpl {

	struct Void_T {};

	template<int... Values>
	struct IntSequence {
		using type = IntSequence<Values...>;
		static constexpr int count = sizeof...(Values);
	};

	template<int Left, typename IntSequence>
	struct IntSequenceSubscript;

	template<int Left, int Value, int... Values>
	struct IntSequenceSubscript<Left, IntSequence<Value, Values...>> {
		static constexpr int value = Left == sizeof...(Values) + 1 ?
			Value :
			IntSequenceSubscript<Left, IntSequence<Values...>>::value;
	};

	template<int Left>
	struct IntSequenceSubscript<Left, IntSequence<>> {
		static constexpr int value = -1;
	};

	template<int Value, typename IntSequence>
	struct AppendIntSequence;

	template<int Value, int... Values>
	struct AppendIntSequence<Value, IntSequence<Values...>> {
		using type = IntSequence<Values..., Value>;
	};

	template<int Count>
	struct MakeIntSequence {
		using type = typename AppendIntSequence<Count - 1, typename MakeIntSequence<Count - 1>::type>::type;
	};

	template<>
	struct MakeIntSequence<1> {
		using type = IntSequence<0>;
	};

	template<typename... Types>
	struct TypeList {
		using type = TypeList<Types...>;
		static constexpr int count = sizeof...(Types);
	};

	template<int I, typename List>
	struct TypeListElement;

	template<int I, typename Head, typename... Tail>
	struct TypeListElement<I, TypeList<Head, Tail...>>
		: TypeListElement<I - 1, TypeList<Tail...>> {
		static constexpr int index = I;
		using type = Head;
	};

	template<typename Head, typename... Tail>
	struct TypeListElement<0, TypeList<Head, Tail...>> {
		static constexpr int index = 0;
		using type = Head;
	};

	template<int I, typename TypeList>
	struct TypeListSubscript {
		using type = typename TypeListElement<I, TypeList>::type;
	};

	template<typename T, typename U>
	struct Match {
		static constexpr bool value = false;
	};

	template<typename T>
	struct Match<T, T> {
		static constexpr bool value = true;
	};


}

template<int Count>
using make_int_sequence = typename mpl::MakeIntSequence<Count>::type;

template<int Index, typename IntSequence>
constexpr int int_sequence_sub_script = mpl::IntSequenceSubscript<IntSequence::count - Index, IntSequence>::value;

template<int I, typename... Types>
using type_list_subscript = typename mpl::TypeListSubscript<mpl::TypeList<Types...>::count - 1 - I, mpl::TypeList<Types...>>::type;

template<int I, typename T>
struct TupleElement {
	T value;
};

template<typename IntList, typename... Types>
struct TupleImpl;

template<int... Indices, typename... Types>
struct TupleImpl<mpl::IntSequence<Indices...>, Types...> : TupleElement<Indices, Types>... {
};

template<typename... Types>
struct Tuple : TupleImpl<make_int_sequence<sizeof...(Types)>, Types...> {

};

template<int I, typename... Types>
constexpr decltype(auto) get(Tuple<Types...> const *tuple) {
	return tuple->TupleElement<I, type_list_subscript<I, Types...>>::value;
}

int main(int argc, char **argv) {
	using array = mpl::IntSequence<10, 45, 16, 26, 19>;
	static_assert(mpl::Match<make_int_sequence<3>, mpl::IntSequence<0, 1, 2>>::value);
	static_assert(int_sequence_sub_script<1, array> == 45);
	static_assert(int_sequence_sub_script<4, array> == 19);
	constexpr Tuple<int, char> tTest{ 4, 'a' };
	static_assert(get<0>(&tTest) == 4);
	static_assert(get<1>(&tTest) == 'a');
}

