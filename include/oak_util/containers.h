#pragma once

#include "types.h"

namespace oak {

	template<typename T>
	struct HashFn {
		u64 operator()(T const& value) {
			static_assert("hash not supported");
			return 0;
		}
	};

	template<typename T>
	struct CmpFn {
		u64 operator()(T const& value) {
			static_assert("compare not supported");
			return 0;
		}
	};

	template<typename K, typename V = void, typename HFn = HashFn<K>, typename CFn = CmpFn<K>>
	struct HashSet {

		using KeyType = K;
		using ValueType = V;

		using HashFn = HFn;
		using CmpFn = CFn;

		SOA<K, V> data;
		i64 count = 0, capacity = 0;
	};

}

