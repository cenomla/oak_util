#pragma once

#include <assert.h>
#include <string.h>
#include <type_traits>
#include <utility>

#include "memory.h"

namespace oak {

	template<typename T>
	struct Delegate;

	template<typename Out, typename... In>
	struct Delegate<Out(In...)> {

		static constexpr u64 DYNAMIC_BIT = u64{ 1 } << 63;

		struct DynamicStorage {
			void *function;
			u64 functionSize;

			DynamicStorage() noexcept = default;
		};

		union {
			char staticStorage[sizeof(DynamicStorage)]{ 0 };
			DynamicStorage dynamicStorage;
		};

		Out (*invokeFn)(void *, In...) = nullptr;

		Delegate() noexcept = default;

		template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Delegate>>>
		Delegate(T&& obj, Allocator *allocator = nullptr) noexcept {
			set(std::forward<T>(obj), allocator);
		}

		template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Delegate>>>
		Delegate& operator=(T&& obj) noexcept {
			set(std::forward<T>(obj));
			return *this;
		}

		Delegate copy_dynamic_storage(Allocator *nAllocator) const noexcept {
			if (!is_dynamic()) {
				return *this;
			}
			// Copy current delegate
			Delegate result = *this;
			void *nFunction = nAllocator->allocate(result.dynamicStorage.functionSize, 8);
			memcpy(nFunction, result.dynamicStorage.function, result.dynamicStorage.functionSize);
			result.dynamicStorage.function = nFunction;

			return result;
		}

		bool is_dynamic() const noexcept {
			return reinterpret_cast<u64>(invokeFn) & DYNAMIC_BIT;
		}

		template<typename T>
		void set(T&& obj, Allocator *allocator = nullptr) noexcept {
			using FT = std::decay_t<T>;

			// Copy the object into the apropriate storage
			if constexpr (sizeof(obj) > sizeof(staticStorage)) {
				assert(allocator);
				dynamicStorage.function = allocate<FT>(allocator, 1);
				dynamicStorage.functionSize = sizeof(obj);
				memcpy(dynamicStorage.function, &obj, sizeof(obj));
			} else {
				memcpy(staticStorage, &obj, sizeof(obj));
			}

			// Set invokeFn based on invoke synatax of the passed functor
			if constexpr (std::is_function_v<FT> || std::is_pointer_v<FT>) {
				invokeFn = [](void *function, In... args) {
					return (*static_cast<FT*>(function))(std::forward<In>(args)...);
				};
			} else {
				invokeFn = [](void *function, In... args) {
					return static_cast<FT*>(function)->operator()(std::forward<In>(args)...);
				};
			}

			assert(!(reinterpret_cast<u64>(invokeFn) & DYNAMIC_BIT) && "invokeFn is at an invalid alignment");

			// If the object is allocated in dynamic storage, set the dynamically allocated bit
			if constexpr(sizeof(obj) > sizeof(staticStorage)) {
				auto invokeInt = reinterpret_cast<u64>(invokeFn);
				invokeInt |= DYNAMIC_BIT;
				invokeFn = reinterpret_cast<decltype(invokeFn)>(invokeInt);
			}
		}

		void destroy(Allocator *allocator) noexcept {
			// If the object was dynamically allocated
			if (is_dynamic()) {
				allocator->deallocate(dynamicStorage.function, dynamicStorage.functionSize);
			}
			invokeFn = nullptr;
		}

		Out operator()(In... args) const noexcept {
			assert(invokeFn);
			auto invokeInt = reinterpret_cast<u64>(invokeFn);
			invokeInt &= ~DYNAMIC_BIT;
			auto realInvoke = reinterpret_cast<decltype(invokeFn)>(invokeInt);
			// Dont return if the return type is void
			if constexpr (std::is_same_v<Out, void>) {
				realInvoke(
						is_dynamic() ? dynamicStorage.function : const_cast<void*>(static_cast<void const*>(staticStorage)),
						std::forward<In>(args)...);
			} else {
				return realInvoke(
						is_dynamic() ? dynamicStorage.function : const_cast<void*>(static_cast<void const*>(staticStorage)),
						std::forward<In>(args)...);
			}
		}

		operator bool() const noexcept {
			return invokeFn != nullptr;
		}

	};

}

