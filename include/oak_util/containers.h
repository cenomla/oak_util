#pragma once

#include "types.h"

namespace oak {

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

