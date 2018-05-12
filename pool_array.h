#pragma once

#include <cstring>
#include <cassert>

#include "allocator.h"
#include "bit.h"

namespace oak {

	template<typename T>
	struct PoolArray {
		struct Iterator {
			Iterator& operator++() {
				idx++;
				return *this;
			}

			inline bool operator==(const Iterator& other) const {
				return data == other.data &&
					poolCapacity == other.poolCapacity &&
					idx == other.idx;
			}

			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			T& operator*() { return data[idx >> log2(poolCapacity)][idx & (poolCapacity - 1)]; }

			T **data = nullptr;
			int64_t poolCapacity = 0;
			int64_t idx = 0;
		};

		typedef T value_type;

		void reserve(int64_t nPools) {
			assert(allocator);
			assert(1lu << log2(poolCapacity) == static_cast<size_t>(poolCapacity));
			if (nPools <= poolCount) { return; }
			auto ndata = static_cast<T**>(allocator->alloc(nPools * sizeof(T*)));
			std::memset(ndata, 0, nPools * sizeof(T*));
			if (data) {
				std::memcpy(ndata, data, poolCount * sizeof(T*));
				allocator->free(data, poolCount * sizeof(T*));
			}
			data = ndata;
			poolCount = nPools;
			//allocate pools
			for (int64_t i = 0; i < poolCount; i++) {
				if (!data[i]) {
					data[i] = static_cast<T*>(allocator->alloc(poolCapacity * sizeof(T)));
				}
			}
		}

		void destroy() {
			if (data) {
				for (int64_t i = 0; i < poolCount; i++) {
					allocator->free(data[i], poolCapacity * sizeof(T));
				}
				allocator->free(data, poolCount * sizeof(T*));
				data = nullptr;
			}
			poolCount = 0;
			size = 0;
		}

		T* push(const T& v) {
			auto capacity = poolCapacity * poolCount;
			if (size == capacity) {
				reserve(poolCount + 1);
			}
			auto& r = operator[](size);
			size++;
			r = v;
			return &r;
		}

		int64_t find(const T& v) {
			for (int64_t i = 0; i < poolCount; i++) {
				for (int64_t j = 0; j < poolCapacity; j++) {
					if (data[i][j] == v) {
						return i * poolCapacity + j;
					}
				}
			}
			return -1;
		}

		T& operator[](int64_t idx) {
			return data[idx >> log2(poolCapacity)][idx & (poolCapacity - 1)];
		}

		const T& operator[](int64_t idx) const {
			return data[idx >> log2(poolCapacity)][idx & (poolCapacity - 1)];
		}

		inline Iterator begin() { return Iterator{ data, poolCapacity, 0 }; }
		inline Iterator end() { return Iterator{ data, poolCapacity, size }; }

		IAllocator *allocator = nullptr;
		int64_t poolCapacity = 0; //must be a power of two
		T **data = nullptr;
		int64_t poolCount = 0;
		int64_t size = 0;

	};
}

