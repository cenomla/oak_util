#pragma once

#include <cassert>
#include <type_traits>
#include <cstring>

#include "allocator.h"

namespace oak {

	template<typename T>
	struct Function {};

	template<typename Out, typename... In>
	struct Function<Out(In...)> {
		IAllocator *allocator = nullptr;
		char staticStorage[16];
		void *function = nullptr;
		Out (*executeFunction)(void *, In&&...);

		template<typename T>
		Function(T&& obj) {
			set(std::forward<T>(obj));
		}

		template<typename T>
		void set(T&& obj) {
			using FT = std::decay_t<T>;
			if constexpr (sizeof(FT) > sizeof(staticStorage)) {
				assert(allocator);
				function = allocator->alloc(sizeof(FT));
			} else {
				function = &staticStorage;
			}
			std::memcpy(function, &obj, sizeof(FT));

			executeFunction = [](void *function, In&&... args) {
				if constexpr (std::is_function_v<std::remove_pointer_t<FT>>) { //check if this is a function pointer type
					return (*static_cast<FT*>(function))(std::forward<In>(args)...);
				} else {
					return static_cast<FT*>(function)->operator()(std::forward<In>(args)...);
				}
			};
		}

		Out operator()(In&&... args) {
			assert(function);
			//dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				executeFunction(function, std::forward<In>(args)...);
			} else {
				return executeFunction(function, std::forward<In>(args)...);
			}
		}

	};

}

