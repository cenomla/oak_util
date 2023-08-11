#pragma once

#include "types.h"
#include "algorithm.h"

namespace oak {

	template<typename First, typename Second>
	struct Pair {
		First first;
		Second second;
	};

	template<typename First, typename Second>
	struct HashFn<Pair<First, Second>> : HashFn<First>, HashFn<Second> {
		constexpr u64 operator()(Pair<First, Second> const& pair) const noexcept {
			return hash_combine(HashFn<First>::operator()(pair.first), HashFn<Second>::operator()(pair.second));
		}
	};

	template<typename First, typename Second>
	struct CmpFn<Pair<First, Second>, Pair<First, Second>> : CmpFn<First, First>, CmpFn<Second, Second> {
		constexpr i64 operator()(Pair<First, Second> const& lhs, Pair<First, Second> const& rhs) const noexcept {
			auto firstCmp = CmpFn<First, First>::operator()(lhs.first, rhs.first);
			return firstCmp != 0 ? firstCmp : CmpFn<Second, Second>::operator()(lhs.second, rhs.second);
		}
	};

}
