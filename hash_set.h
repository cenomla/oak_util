#pragma once

#include <cstring>
#include <cassert>
#include <limits>

#include "allocator.h"
#include "bit.h"
#include "ptr.h"

namespace oak {

	constexpr size_t hash(const size_t& v) {
		return v;
	}

	template<typename V>
	struct HashSet {

		struct Iterator {
			Iterator& operator++() {
				do {
					idx ++;
					if (idx == set->capacity) { return *this; }
				} while (!set->hashs[idx]);
				return *this;
			}

			inline bool operator==(const Iterator& other) const { return set == other.set && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			V& operator*() { return set->values[idx]; }

			const HashSet *set;
			size_t idx;
		};

		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef V value_type;

		void resize(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			//make nsize a power of two
			nsize = next_pow2(nsize);

			HashSet nset{ allocator };
			auto count = nsize * (sizeof(V) + sizeof(size_t));
			auto mem = allocator->alloc(count);
			std::memset(mem, 0, count);
			nset.values = static_cast<V*>(mem);
			nset.hashs = static_cast<size_t*>(ptr::add(mem, nsize * sizeof(V)));
			nset.size = 0;
			nset.capacity = nsize;
			nset.firstIndex = nsize;

			size_t left = size;
			for (size_t i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i]) {
					nset.put(values[i]);
					left--;
				}
			}

			destroy();
			std::memcpy(this, &nset, sizeof(HashSet));
		}

		HashSet clone() {
			HashSet nset{ allocator };
			nset.resize(capacity);

			size_t left = size;
			for (size_t i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i]) {
					nset.put(values[i]);
					left--;
				}
			}

			return nset;
		}

		void destroy() {
			if (values) {
				allocator->free(values, capacity * (sizeof(V) + sizeof(size_t)));
				values = nullptr;
				hashs = nullptr;
			}
			size = 0;
			capacity = 0;
			firstIndex = 0;
		}

		size_t find(const V& value) {
			if (size == 0) { return npos; }
			auto h = hash(value);
			auto idx = h & (capacity - 1);
			auto firstTaken = hashs[idx] ? true : false;
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (firstTaken && !hashs[ridx]) {
					return npos;
				}

				if (hashs[ridx] == h && values[ridx] == value) {
					return ridx;
				}
			}
			return npos;
		}

		size_t find_hash(size_t h) {
			if (size == 0) { return npos; }
			auto idx = h & (capacity - 1);
			auto firstTaken = hashs[idx] ? true : false;
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (firstTaken && !hashs[ridx]) {
					return npos;
				}

				if (hashs[ridx] == h) {
					return ridx;
				}
			}
			return npos;
		}

		bool has(const V& value) {
			return find(value) != npos;
		}

		V* get(const V& value) {
			if (capacity == 0) { return nullptr; }
			auto idx = find(value);
			return idx != npos ? value + idx : nullptr;
		}

		V* put(const V& value) {
			if (size == capacity) {
				resize(capacity == 0 ? 4 : capacity * 2);
			}
			auto h = hash(value);
			auto idx = h & (capacity - 1);
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (hashs[ridx]) {
					//first check if the hash is equivalent
					if (hashs[ridx] == h && values[ridx] == value) {
						values[ridx] = value;
						return values + ridx;
					}
				} else {
					values[ridx] = value;
					hashs[ridx] = h;
					if (ridx < firstIndex) {
						firstIndex = ridx;
					}
					size++;

					return values + ridx;
				}
			}
			return nullptr;
		}

		void remove(size_t idx) {
			if (idx == npos) { return; }
			hashs[idx] = 0;
			size --;
			if (firstIndex == idx) { //calculate new first index
				firstIndex = capacity;
				for (auto i = 0u; i < capacity; i++) {
					if (hashs[i]) {
						firstIndex = i;
						break;
					}
				}
			}
		}

		inline Iterator begin() const { return Iterator{ this, firstIndex }; }
		inline Iterator end() const { return Iterator{ this, capacity }; }

		IAllocator *allocator = nullptr;
		V *values = nullptr;
		size_t *hashs = nullptr;

		size_t size = 0, capacity = 0, firstIndex = 0;
	};

}

