#pragma once

#include <new>

#include "allocator.h"

namespace oak {

	template<typename T, typename... TArgs>
	T* make(IAllocator *allocator, TArgs&&... parameters) {
		auto mem = allocator->alloc(sizeof(T));
		new (mem) T{ parameters... };
		return static_cast<T*>(mem);
	}

	template<typename T, typename... TArgs>
	T* make_array(IAllocator *allocator, size_t count, TArgs&&... parameters) {
		auto mem = static_cast<T*>(allocator->alloc(sizeof(T) * count));
		for (size_t i = 0; i < count; i++) {
			new (mem + i) T { parameters... };
		}
		return mem;
	}

}
