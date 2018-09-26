#pragma once

#include <cassert>
#include <cstring>
#include <type_traits>

#include "allocator.h"
#include "memory.h"

namespace oak {

	template<typename T>
	struct Function {};

	template<typename Out, typename... In>
	struct Function<Out(In...)> {
		MemoryArena *arena = nullptr;
		union {
			char staticStorage[16]{ 0 };
			size_t functionSize;
		};
		void *function = nullptr;
		Out (*executeFunction)(void *, In&...) = nullptr;

		Function() = default;

		template<typename T>
		Function(T&& obj) {
			set(std::forward<T>(obj));
		}

		template<typename T>
		Function(MemoryArena *arena_, T&& obj) : arena{ arena_ } {
			set(std::forward<T>(obj));
		}

		template<typename T>
		void set(T&& obj) {
			using FT = std::decay_t<T>;
			if constexpr (sizeof(obj) > sizeof(staticStorage)) {
				if (arena) {
					function = allocate_structs<T>(arena, 1);
				} else {
					function = alloc(sizeof(obj));
				}
				functionSize = sizeof(obj);
			} else {
				function = &staticStorage;
			}
			std::memcpy(function, &obj, sizeof(obj));

			// Check if this is a function pointer type
			if constexpr (std::is_function_v<std::remove_pointer_t<FT>>) {
				executeFunction = [](void *function, In&... args) {
					return (*static_cast<FT*>(function))(args...);
				};
			} else {
				executeFunction = [](void *function, In&... args) {
					return static_cast<FT*>(function)->operator()(args...);
				};
			}
		}

		void destroy() {
			// If the object is empty
			if (!function) { return; }
			// If the object was dynamically allocated
			if (function != &staticStorage) {
				assert(functionSize);
				if (arena) {
					// We dont free from memory arenas
				} else {
					free(function, functionSize);
				}
			}
			function = nullptr;
		}

		Out operator()(In&... args) {
			assert(function);
			assert(executeFunction);
			// Dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				executeFunction(function, args...);
			} else {
				return executeFunction(function, args...);
			}
		}

	};

}

