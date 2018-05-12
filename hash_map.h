#pragma once

#include <cstring>
#include <cassert>

#include "allocator.h"
#include "bit.h"
#include "cmp.h"
#include "hash.h"
#include "ptr.h"

namespace oak {

	template<typename K, typename V>
	struct HashMap {

		struct Pair {
			K *key;
			V *value;
		};

		static constexpr size_t EMPTY_HASH = ~size_t{ 0 };

		struct Iterator {
			Iterator& operator++() {
				do {
					idx ++;
					if (idx == map->capacity) { return *this; }
				} while (map->hashs[idx] == EMPTY_HASH);
				return *this;
			}

			inline bool operator==(const Iterator& other) const { return map == other.map && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			Pair operator*() { return { map->keys + idx, map->values + idx }; }

			const HashMap *map;
			int64_t idx;
		};

		typedef K key_type;
		typedef V value_type;

		void resize(int64_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			//make nsize a power of two
			nsize = ensure_pow2(nsize);

			HashMap nmap{ allocator };
			auto count = nsize * (sizeof(K) + sizeof(V) + sizeof(size_t));
			auto mem = allocator->alloc(count);
			nmap.keys = static_cast<K*>(mem);
			nmap.values = static_cast<V*>(ptr::add(mem, nsize * sizeof(K)));
			nmap.hashs = static_cast<size_t*>(ptr::add(mem, nsize * (sizeof(K) + sizeof(V))));
			std::memset(nmap.keys, 0, nsize * sizeof(K));
			std::memset(nmap.values, 0, nsize * sizeof(V));
			std::memset(nmap.hashs, 0xFF, nsize * sizeof(size_t));
			assert(nmap.hashs[0] == EMPTY_HASH);
			nmap.size = 0;
			nmap.capacity = nsize;
			nmap.firstIndex = nsize;

			auto left = size;
			for (auto i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i] != EMPTY_HASH) {
					nmap.put(keys[i], values[i]);
					left--;
				}
			}

			destroy();
			std::memcpy(this, &nmap, sizeof(HashMap));
		}

		HashMap clone() {
			HashMap nmap{ allocator };
			nmap.resize(capacity);

			auto left = size;
			for (auto i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i] != EMPTY_HASH) {
					nmap.put(keys[i], values[i]);
					left--;
				}
			}

			return nmap;
		}

		void destroy() {
			if (keys) {
				allocator->free(keys, capacity * (sizeof(K) + sizeof(V) + sizeof(size_t)));
				keys = nullptr;
				values = nullptr;
				hashs = nullptr;
			}
			size = 0;
			capacity = 0;
			firstIndex = 0;
		}

		int64_t find(const K& key) const {
			if (size == 0) { return -1; }
			auto h = HashFunc<K>{}(key);
			int64_t idx = h & (capacity - 1);
			auto firstTaken = hashs[idx] != EMPTY_HASH;
			for (int64_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (firstTaken && hashs[ridx] == EMPTY_HASH) {
					return -1;
				}

				if (hashs[ridx] == h && CmpFunc<K, K>{}(keys[ridx], key)) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_hash(size_t h) const {
			if (size == 0) { return -1; }
			int64_t idx = h & (capacity - 1);
			auto firstTaken = hashs[idx] != EMPTY_HASH;
			for (int64_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (firstTaken && hashs[ridx] == EMPTY_HASH) {
					return -1;
				}

				if (hashs[ridx] == h) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_value(const V& value) const {
			for (int64_t i = 0; i < size; i++) {
				if (hashs[i] != EMPTY_HASH && CmpFunc<V, V>{}(values[i], value)) {
					return i;
				}
			}
			return -1;
		}

		bool has(const K& key) const {
			return find(key) != -1;
		}

		V* get(const K& key) {
			if (capacity == 0) { return nullptr; }
			auto idx = find(key);
			return idx != -1 ? values + idx : nullptr;
		}

		V* put(const K& key, const V& value) {
			if (size == capacity) {
				resize(capacity == 0 ? 4 : capacity * 2);
			}
			auto h = HashFunc<K>{}(key);
			int64_t idx = h & (capacity - 1);
			for (int64_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (hashs[ridx] != EMPTY_HASH) {
					if (hashs[ridx] == h && CmpFunc<K, K>{}(keys[ridx], key)) {
						values[ridx] = value;
						return values + ridx;
					}
				} else {
					keys[ridx] = key;
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

		void remove(int64_t idx) {
			assert(idx >= 0);
			assert(size > 0);
			hashs[idx] = EMPTY_HASH;
			size--;
			if (firstIndex == idx) { //calculate new first index
				firstIndex = capacity;
				for (int64_t i = 0; i < capacity; i++) {
					if (hashs[i] != EMPTY_HASH) {
						firstIndex = i;
						break;
					}
				}
			}
		}

		inline Iterator begin() const { return Iterator{ this, firstIndex }; }
		inline Iterator end() const { return Iterator{ this, capacity }; }

		IAllocator *allocator = nullptr;
		K *keys = nullptr;
		V *values = nullptr;
		size_t *hashs = nullptr;

		int64_t size = 0, capacity = 0, firstIndex = 0;
	};

}

