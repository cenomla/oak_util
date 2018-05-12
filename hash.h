#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace oak {

	template<typename T>
	struct HashFunc {
		constexpr size_t operator()(const T& v) const {
			return ~size_t{ 0 };
		}
	};

	template<>
	struct HashFunc<uint64_t> {
		constexpr size_t operator()(const uint64_t& v) const {
			return v;
		}
	};

	template<>
	struct HashFunc<int64_t> {
		constexpr size_t operator()(const int64_t& v) const {
			return static_cast<size_t>(v + (std::numeric_limits<int32_t>::max() - 1));
		}
	};

}

