#pragma once

#include <cstring>
#include <cassert>
#include <initializer_list>

#include "allocator.h"
#include "slice.h"
#include "bit.h"

namespace oak {

	template<typename T>
	struct Array {
		typedef T value_type;

		Array() = default;
		Array(IAllocator *_allocator) : allocator{ _allocator } {}
		Array(IAllocator *_allocator, const Slice<T>& other) : allocator{ _allocator } {
			reserve(other.count);
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			std::memcpy(mem, std::begin(other), other.count * sizeof(T));
		}
		Array(IAllocator *_allocator, std::initializer_list<T> list) : allocator{ _allocator } {
			reserve(list.count());
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			std::memcpy(mem, std::begin(list), list.count() * sizeof(T));
		}

		void reserve(int64_t ncount) {
			assert(allocator);
			if (ncount <= capacity) { return; }
			ncount = ensure_pow2(ncount);
			auto mem = static_cast<std::remove_const_t<T>*>(allocator->alloc(ncount * sizeof(T)));
			if (data) {
				std::memcpy(mem, data, capacity * sizeof(T));
				allocator->free(data, capacity * sizeof(T));
			}
			data = mem;
			capacity = ncount;
		}

		void resize(int64_t ncount) {
			reserve(ncount);
			count = ncount;
		}

		void resize(int64_t ncount, const T& value) {
			reserve(ncount);
			auto mem = const_cast<std::remove_const_t<T>*>(data);
			for (auto i = count; i < ncount; i++) {
				mem[i] = T{ value };
			}
			count = ncount;
		}

		constexpr int64_t find(const T& v, int64_t start = 0) const {
			if (!data) { return -1; }
			for (auto i = start; i < count; i++) {
				if (data[i] == v) {
					return i;
				}
			}
			return -1;
		}

		Array clone(IAllocator *oAllocator = nullptr) const {
			if (!oAllocator) {
				oAllocator = allocator;
			}
			Array narry{ oAllocator };
			narry.reserve(capacity);
			narry.count = count;
			std::memcpy(narry.data, data, capacity * sizeof(T));
			return narry;
		}

		void destroy() {
			if (data) {
				allocator->free(data, capacity * sizeof(T));
				data = nullptr;
			}
			count = 0;
			capacity = 0;
		}

		T* push(const T& v) {
			if (count == capacity) {
				reserve(capacity == 0 ? 4 : capacity * 2);
			}
			data[count++] = v;
			return data + count - 1;
		}

		T* insert(const T& v, int64_t idx) {
			if (idx == -1 || idx == count) {
				return push(v);
			}
			resize(count + 1);
			std::memmove(data + idx + 1, data + idx, (count - 1 - idx) * sizeof(T));
			data[idx] = v;
			return data + idx;
		}

		void remove(int64_t idx) {
			assert(0 <= idx && idx < count);
			data[idx] = data[--count];
		}

		void remove_ordered(int64_t idx) {
			assert(idx < count);
			//move the upper portion or the array down one index
			std::memmove(data + idx, data + idx + 1, (count - 1 - idx) * sizeof(T));
			count--;
		}

		constexpr T& operator[](int64_t idx) { return data[idx]; }
		constexpr const T& operator[](int64_t idx) const { return data[idx]; }

		inline T* begin() const { return data; }
		inline T* end() const { return data + count; }
		inline const T* cbegin() const { return data; }
		inline const T* cend() const { return data + count; }

		operator Slice<T>() const { return Slice<T>{ data, count }; }

		IAllocator *allocator = nullptr;
		T *data = nullptr;
		int64_t count = 0;
		int64_t capacity = 0;
	};

}

