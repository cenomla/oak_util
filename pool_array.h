#pragma once

#include <cstring>
#include <cassert>

#include "allocator.h"

namespace oak {

	template<typename T>
	struct PoolArray {
		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef T value_type; 

		void reserve(size_t nPools) {
			assert(allocator);
			if (nPools <= poolCount) { return; }
			auto ndata = static_cast<T**>(allocator->alloc(nPools * sizeof(T*)));
			std::memset(ndata, 0, poolCount * sizeof(T*));
			if (data) {
				std::memcpy(ndata, data, poolCount * sizeof(T*));
				allocator->free(data, poolCount * sizeof(T*));
			}
			data = ndata;
			poolCount = nPools;
			//allocate pools
			for (size_t i = 0; i < poolCount; i++) {
				if (!data[i]) {
					data[i] = static_cast<T*>(allocator->alloc((1 << poolCapacityLog) * sizeof(T)));
				}
			}
		}

		T* push(const T& v) {
			auto capacity = (1 << poolCapacityLog) * poolCount;
			if (size == capacity) {
				reserve(poolCount + 1);
			}
			auto& r = operator[](size);
			size++;
			r = v;
			return &r;
		}

		size_t find(const T& v) {
			for (size_t i = 0; i < poolCount; i++) {
				for (size_t j = 0; j < (1 << poolCapacityLog); j++) {
					if (data[i][j] == v) {
						return i * (1 << poolCapacityLog) + j;
					}
				}
			}
			return npos;
		}

		T& operator[](size_t idx) {
			return data[idx >> poolCapacityLog][idx & ((1 << poolCapacityLog) - 1)];
		}

		const T& operator[](size_t idx) const {
			return data[idx >> poolCapacityLog][idx & ((1 << poolCapacityLog) - 1)];
		}

		IAllocator *allocator = nullptr;
		size_t poolCapacityLog = 0; //log2 of pool capacity
		T **data = nullptr;
		size_t poolCount = 0;
		size_t size = 0;

	};
}
