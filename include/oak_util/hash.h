#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace oak {

	template<typename T>
	struct HashFunc {
		constexpr size_t operator()(T const& v) const {
			static_assert(std::is_integral_v<T> || std::is_pointer_v<T>);
			if constexpr (std::is_integral_v<T>) {
				return static_cast<size_t>(v);
			} else if constexpr (std::is_pointer_v<T>) {
				return reinterpret_cast<size_t>(v);
			}
		}

	};

}

