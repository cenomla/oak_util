#pragma once

#include <cassert>
#include <cstring>
#include <type_traits>
#include <utility>

#include "memory.h"

namespace oak {

	constexpr size_t SMALL_FUNCTION_SIZE = 16;

	template<typename T>
	struct Delegate;

	template<typename Out, typename... In>
	struct Delegate<Out(In...)> {

		union {
			char staticStorage[SMALL_FUNCTION_SIZE]{ 0 };
			size_t functionSize;
		};

		Allocator *allocator = nullptr;
		void *function = nullptr;
		Out (*invokeFn)(void *, In...) = nullptr;

		Delegate() noexcept = default;

		Delegate(Allocator *allocator_) noexcept
			: allocator{ allocator_ } {}

		Delegate(Delegate const& other) noexcept = delete;
		Delegate& operator=(Delegate const& other) noexcept = delete;

		Delegate(Delegate&& other) noexcept {
			operator=(static_cast<Delegate&&>(other));
		}

		Delegate& operator=(Delegate&& other) noexcept {
			allocator = other.allocator;
			if (other.function == &other.staticStorage) {
				function = &staticStorage;
				std::memcpy(staticStorage, other.staticStorage, sizeof(staticStorage));
			} else {
				function = other.function;
				functionSize = other.functionSize;
			}

			invokeFn = other.invokeFn;

			other.allocator = nullptr;
			std::memset(other.staticStorage, 0, sizeof(other.staticStorage));
			other.function = nullptr;
			other.invokeFn = nullptr;
			return *this;
		}

		template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Delegate>>>
		Delegate(T&& obj) noexcept {
			set(std::forward<T>(obj));
		}

		template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Delegate>>>
		Delegate(Allocator *allocator_, T&& obj) noexcept
			: allocator{ allocator_ } {
			set(std::forward<T>(obj));
		}

		template<typename T>
		void set(T&& obj) noexcept {
			using FT = std::decay_t<T>;
			static_assert(!std::is_function_v<FT> && "Cannot pass function into delegate, must pass function pointer");
			if constexpr (sizeof(obj) > sizeof(staticStorage)) {
				assert(allocator);
				function = allocate<T>(allocator, 1);
				functionSize = sizeof(obj);
			} else {
				function = staticStorage;
			}
			std::memcpy(function, &obj, sizeof(obj));

			// Check if this is a function pointer type
			if constexpr (std::is_function_v<std::remove_pointer_t<FT>>) {
				invokeFn = [](void *function, In... args) {
					return (*static_cast<FT*>(function))(std::forward<In>(args)...);
				};
			} else {
				invokeFn = [](void *function, In... args) {
					return static_cast<FT*>(function)->operator()(std::forward<In>(args)...);
				};
			}
		}

		void destroy() noexcept {
			// If the object is empty
			if (!function) { return; }
			// If the object was dynamically allocated
			if (function != &staticStorage) {
				assert(functionSize);
				allocator->deallocate(function, functionSize);
			}
			function = nullptr;
		}

		Out operator()(In... args) const noexcept {
			assert(function);
			assert(invokeFn);
			// Dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				invokeFn(function, std::forward<In>(args)...);
			} else {
				return invokeFn(function, std::forward<In>(args)...);
			}
		}

		operator bool() const noexcept {
			return function != nullptr;
		}

	};

}

