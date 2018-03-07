#pragma once

#include <cstring>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <new>

#include "allocator.h"
#include "slice.h"

namespace oak {

	template<typename T>
	struct Array : Slice<T> {

		using Slice<T>::Slice;
		using Slice<T>::operator=;

		Array() = default;
		Array(IAllocator *_allocator) : Slice<T>{}, allocator{ _allocator } {}
		Array(IAllocator *_allocator, const Slice<T>& other) : Slice<T>{}, allocator{ _allocator } {
			reserve(other.size);
			auto mem = const_cast<std::remove_const_t<T>*>(Slice<T>::data);
			std::memcpy(mem, std::begin(other), other.size * sizeof(T));
		}
		Array(IAllocator *_allocator, std::initializer_list<T> list) : Slice<T>{}, allocator{ _allocator } {
			reserve(list.size());
			auto mem = const_cast<std::remove_const_t<T>*>(Slice<T>::data);
			std::memcpy(mem, std::begin(list), list.size() * sizeof(T));
		}

		void reserve(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			auto mem = static_cast<std::remove_const_t<T>*>(allocator->alloc(nsize * sizeof(T)));
			if (Slice<T>::data) {
				std::memcpy(mem, Slice<T>::data, capacity * sizeof(T));
				allocator->free(Slice<T>::data, capacity * sizeof(T));
			}
			Slice<T>::data = mem;
			capacity = nsize;
		}

		void resize(size_t nsize) {
			reserve(nsize);
			Slice<T>::size = nsize;
		}

		void resize(size_t nsize, const T& value) {
			reserve(nsize);
			auto mem = const_cast<std::remove_const_t<T>*>(Slice<T>::data);
			for (auto i = Slice<T>::size; i < nsize; i++) {
				new (mem + i) T{ value };
			}
			Slice<T>::size = nsize;
		}

		Array clone(IAllocator *oAllocator = nullptr) const {
			if (!oAllocator) {
				oAllocator = allocator;
			}
			Array narry{ oAllocator };
			narry.reserve(capacity);
			narry.size = Slice<T>::size;
			std::memcpy(narry.data, Slice<T>::data, capacity * sizeof(T));
			return narry;
		}

		void destroy() {
			if (Slice<T>::data) {
				allocator->free(Slice<T>::data, capacity * sizeof(T));
				Slice<T>::data = nullptr;
			}
			Slice<T>::size = 0;
			capacity = 0;
		}

		T* push(const T& v) {
			if (Slice<T>::size == capacity) {
				reserve(capacity == 0 ? 4 : capacity * 2);
			}
			Slice<T>::data[Slice<T>::size++] = v;
			return Slice<T>::data + Slice<T>::size - 1;
		}

		T* insert(const T& v, size_t idx) {
			if (idx == Slice<T>::npos || idx == Slice<T>::size) {
				return push(v);
			}
			resize(Slice<T>::size + 1);
			std::memmove(Slice<T>::data + idx + 1, Slice<T>::data + idx, (Slice<T>::size - 1 - idx) * sizeof(T));
			Slice<T>::data[idx] = v;
			return Slice<T>::data + idx;
		}

		void remove(size_t idx) {
			assert(idx < Slice<T>::size);
			//swap and pop
			T& v = Slice<T>::data[Slice<T>::size - 1];
			Slice<T>::data[idx] = v;
			Slice<T>::size--;
		}

		void remove_ordered(size_t idx) {
			assert(idx < Slice<T>::size);
			//move the upper portion or the array down one index
			std::memmove(Slice<T>::data + idx, Slice<T>::data + idx + 1, (Slice<T>::size - 1 - idx) * sizeof(T));
			Slice<T>::size--;
		}

		IAllocator *allocator = nullptr;
		size_t capacity = 0;
	};

}

