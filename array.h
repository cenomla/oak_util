#pragma once

#include <cstring>
#include <cassert>
#include <initializer_list>
#include <new>

#include "allocator.h"
#include "slice.h"

namespace oak {

	template<typename T>
	struct Array {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef T value_type; 

		Array() = default;
		Array(IAllocator *_allocator) : allocator{ _allocator } {}
		Array(IAllocator *_allocator, const Slice<T>& other) : allocator{ _allocator } {
			reserve(other.size);
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			std::memcpy(mem, std::begin(other), other.size * sizeof(T));
		}
		Array(IAllocator *_allocator, std::initializer_list<T> list) : allocator{ _allocator } {
			reserve(list.size());
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			std::memcpy(mem, std::begin(list), list.size() * sizeof(T));
		}

		void reserve(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			auto mem = static_cast<std::remove_const_t<T>*>(allocator->alloc(nsize * sizeof(T)));
			if (data) {
				std::memcpy(mem, data, capacity * sizeof(T));
				allocator->free(data, capacity * sizeof(T));
			}
			data = mem;
			capacity = nsize;
		}

		void resize(size_t nsize) {
			reserve(nsize);
			size = nsize;
		}

		void resize(size_t nsize, const T& value) {
			reserve(nsize);
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			for (auto i = size; i < nsize; i++) {
				new (mem + i) T{ value };
			}
			size = nsize;
		}

		constexpr size_t find(const T& v, size_t start = 0) const {
			if (!data) { return npos; }
			for (size_t i = start; i < size; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return npos;
		}

		Array clone(IAllocator *oAllocator = nullptr) const {
			if (!oAllocator) {
				oAllocator = allocator;
			}
			Array narry{ oAllocator };
			narry.reserve(capacity);
			narry.size = size;
			std::memcpy(narry.data, data, capacity * sizeof(T));
			return narry;
		}

		void destroy() {
			if (data) {
				allocator->free(data, capacity * sizeof(T));
				data = nullptr;
			}
			size = 0;
			capacity = 0;
		}

		T* push(const T& v) {
			if (size == capacity) {
				reserve(capacity == 0 ? 4 : capacity * 2);
			}
			data[size++] = v;
			return data + size - 1;
		}

		T* insert(const T& v, size_t idx) {
			if (idx == npos || idx == size) {
				return push(v);
			}
			resize(size + 1);
			std::memmove(data + idx + 1, data + idx, (size - 1 - idx) * sizeof(T));
			data[idx] = v;
			return data + idx;
		}

		void remove(size_t idx) {
			assert(idx < size);
			//swap and pop
			T& v = data[size - 1];
			data[idx] = v;
			size--;
		}

		void remove_ordered(size_t idx) {
			assert(idx < size);
			//move the upper portion or the array down one index
			std::memmove(data + idx, data + idx + 1, (size - 1 - idx) * sizeof(T));
			size--;
		}

		operator Slice<T>() const { return Slice<T>{ data, size }; }

		IAllocator *allocator = nullptr;
		T *data = nullptr;
		size_t size = 0;
		size_t capacity = 0;
	};

}

