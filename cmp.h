#pragma once

#include <cstdint>
#include <limits>

namespace oak {

	template<typename T, typename U>
	struct CmpFunc {
		constexpr bool operator()(const T& lhs, const U& rhs) const {
			return lhs == rhs;
		}
	};

}

