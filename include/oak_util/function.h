#pragma once

#include <cassert>
#include <cstring>
#include <type_traits>

#include "memory.h"

namespace oak {

	constexpr size_t SMALL_FUNCTION_SIZE = 16;

	template<typename T>
	struct Function {};

	template<typename Out, typename... In>
	struct Function<Out(In...)> {
		MemoryArena *arena = nullptr;
		union {
			char staticStorage[SMALL_FUNCTION_SIZE]{ 0 };
			size_t functionSize;
		};
		void *function = nullptr;
		Out (*executeFunction)(void *, In&...) = nullptr;

		Function() = default;

		Function(Function&& other) {
			operator=(static_cast<Function&&>(other));
		}

		Function& operator=(Function&& other) {
			arena = other.arena;
			if (other.function == &other.staticStorage) {
				function = &staticStorage;
				std::memcpy(staticStorage, other.staticStorage, sizeof(staticStorage));
			} else {
				function = other.function;
				functionSize = other.functionSize;
			}

			executeFunction = other.executeFunction;

			other.arena = nullptr;
			std::memset(other.staticStorage, 0, sizeof(other.staticStorage));
			other.function = nullptr;
			other.executeFunction = nullptr;
			return *this;
		}

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
					function = allocate<T>(arena, 1);
				} else {
					function = alloc(sizeof(obj));
				}
				functionSize = sizeof(obj);
			} else {
				function = staticStorage;
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

		Out operator()(In... args) const {
			assert(function);
			assert(executeFunction);
			// Dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				executeFunction(function, args...);
			} else {
				return executeFunction(function, args...);
			}
		}

		operator bool() const {
			return function != nullptr;
		}

	};

}

