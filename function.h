#pragma once

#include <cassert>
#include <cstring>
#include <type_traits>

#include "allocator.h"

namespace oak {

	template<typename T>
	struct Function {};

	template<typename Out, typename... In>
	struct Function<Out(In...)> {
		IAllocator *allocator = nullptr;
		union {
			char staticStorage[16]{ 0 };
			size_t functionSize;
		};
		void *function = nullptr;
		Out (*executeFunction)(void *, In&&...) = nullptr;

		Function() = default;

		template<typename T>
		Function(T&& obj) {
			set(std::forward<T>(obj));
		}

		template<typename T>
		void set(T&& obj) {
			using FT = std::decay_t<T>;
			if constexpr (sizeof(obj) > sizeof(staticStorage)) {
				assert(allocator);
				function = allocator->alloc(sizeof(obj));
				functionSize = sizeof(obj);
			} else {
				function = &staticStorage;
			}
			std::memcpy(function, &obj, sizeof(obj));

			executeFunction = [](void *function, In&&... args) {
				if constexpr (std::is_function_v<std::remove_pointer_t<FT>>) { //check if this is a function pointer type
					return (*static_cast<FT*>(function))(std::forward<In>(args)...);
				} else {
					return static_cast<FT*>(function)->operator()(std::forward<In>(args)...);
				}
			};
		}

		void destroy() {
			//if the object is empty
			if (!function) { return; }
			//if the object was dynamically allocated
			if (function != &staticStorage) {
				assert(allocator);
				assert(functionSize);
				allocator->free(function, functionSize);
			}
			function = nullptr;
		}

		Out operator()(In&&... args) {
			assert(function);
			assert(executeFunction);
			//dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				executeFunction(function, std::forward<In>(args)...);
			} else {
				return executeFunction(function, std::forward<In>(args)...);
			}
		}

	};

}

