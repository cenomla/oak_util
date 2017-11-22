#pragma once

#include <cstring>
#include <cassert>

#include "allocator.h"

namespace oak {

	template<class T>
	struct Array {

		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef T value_type; 

		void reserve(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			auto mem = static_cast<T*>(allocator->allocate(nsize * sizeof(T)));
			if (data) {
				std::memcpy(mem, data, capacity * sizeof(T));
				allocator->deallocate(data, capacity * sizeof(T));
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
			for (auto i = size; i < nsize; i++) {
				data[i] = value;
			}
			size = nsize;
		}

		Array clone() {
			Array nmap{ allocator };
			nmap.resize(size);
			std::memcpy(nmap.data, data, capacity * sizeof(T));
			return nmap;
		}

		void destroy() {
			if (data) {
				allocator->deallocate(data, capacity * sizeof(T));
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

		size_t find(const T& v) {
			for (size_t i = 0; i < size; i++) {
				if (std::memcmp(data + i, &v, sizeof(T)) == 0) {
					return i;
				}
			}
			return npos;
		}

		void remove(size_t index) {
			//swap and pop
			T& v = data[size - 1];
			data[index] = v;
			size--;
		}

		T& operator[](size_t index) { return data[index]; }
		const T& operator[](size_t index) const { return data[index]; }

		inline T* begin() { return data; }
		inline T* end() { return data + size; }

		IAllocator *allocator = nullptr;
		T *data = nullptr;
		size_t size = 0;
		size_t capacity = 0;
	};

}

