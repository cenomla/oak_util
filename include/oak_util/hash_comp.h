#pragma once

#include <cinttypes>
#include <cstddef>
#include <cassert>

namespace oak {

	template<typename K, typename Impl>
	struct HashSetBase {

	};

	template<typename K, typename V, typename Impl>
	struct HashMapBase : HashSetBase<K, Impl> {

	};

	template<typename Impl, typename... Args>
	struct DynamicHashContainerBase {
		void resize(int64_t capacity_) {
		//	auto mem = Impl::allocator.alloc(capacity * (sizeof(Args)... +));
		}

		void destroy() {
		}

	};

}

