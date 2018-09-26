#pragma once

namespace oak {

	template<typename T, typename U>
	struct EqualFunc {
		constexpr int operator()(const T& lhs, const U& rhs) const {
			return lhs == rhs;
		}
	};

}

