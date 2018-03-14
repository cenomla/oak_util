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

	template<typename K, typename V>
	struct HashMap {

		struct Pair {
			K *key;
			V *value;
		};

		struct Iterator {
			Iterator& operator++() {
				do {
					idx ++;
					if (idx == map->capacity) { return *this; }
				} while (!map->hashs[idx]);
				return *this;
			}	

			inline bool operator==(const Iterator& other) const { return map == other.map && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			Pair operator*() { return { map->keys + idx, map->values + idx }; }

			const HashMap *map;
			size_t idx;
		};

		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef K key_type;
		typedef V value_type;

		void resize(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }
			//make nsize a power of two
			nsize = npow2(nsize);

			HashMap nmap{ allocator };
			auto count = nsize * (sizeof(K) + sizeof(V) + sizeof(size_t)); 
			auto mem = allocator->alloc(count);
			std::memset(mem, 0, count);
			nmap.keys = static_cast<K*>(mem);
			nmap.values = static_cast<V*>(ptr::add(mem, nsize * sizeof(K)));
			nmap.hashs = static_cast<size_t*>(ptr::add(mem, nsize * (sizeof(K) + sizeof(V))));
			nmap.size = 0;
			nmap.capacity = nsize;
			nmap.firstIndex = nsize;
			
			size_t left = size;
			for (size_t i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i]) {
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
			
			size_t left = size;
			for (size_t i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i]) {
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

		size_t find(const K& key) {
			if (size == 0) { return npos; }
			auto h = hash(key);
			auto idx = h & (capacity - 1);
			auto firstTaken = hashs[idx] ? true : false;
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (firstTaken && !hashs[ridx]) {
					return npos;
				}

				if (hashs[ridx] == h && keys[ridx] == key) {
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

		size_t find_value(const V& value) {
			for (size_t i = 0; i < size; i++) {
				if (hashs[i] && values[i] == value) {
					return i;
				}
			}
			return npos;
		}

		bool has(const K& key) {
			return find(key) != npos;
		}
			
		V* get(const K& key) {
			if (capacity == 0) { return nullptr; }
			auto idx = find(key);
			return idx != npos ? values + idx : nullptr; 
		}

		V* put(const K& key, const V& value) {
			if (size == capacity) {
				resize(capacity == 0 ? 4 : capacity * 2);
			}
			auto h = hash(key);
			auto idx = h & (capacity - 1);
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (hashs[ridx]) {
					//first check if the hash is equivalent
					if (hashs[ridx] == h && keys[ridx] == key) {
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
		K *keys = nullptr;
		V *values = nullptr;
		size_t *hashs = nullptr;

		size_t size = 0, capacity = 0, firstIndex = 0;
	};

}

