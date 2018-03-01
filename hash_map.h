#pragma once

#include <cstring>
#include <cassert>

#include "allocator.h"
#include "ptr.h"

namespace oak {

	constexpr size_t hash(const size_t& v) {
		return v;
	}

	template<class K, class V>
	struct HashMap {

		static constexpr size_t npos = 0xFFFFFFFFFFFFFFFF;

		typedef K key_type;
		typedef V value_type;

		void resize(size_t nsize) {
			assert(allocator);
			if (nsize <= capacity) { return; }

			HashMap nmap{ allocator };
			auto count = nsize * (sizeof(K) + sizeof(V) + sizeof(bool)); 
			auto mem = allocator->alloc(count);
			std::memset(mem, 0, count);
			nmap.keys = static_cast<K*>(mem);
			nmap.values = static_cast<V*>(ptr::add(mem, nsize * sizeof(K)));
			nmap.taken = static_cast<bool*>(ptr::add(mem, nsize * (sizeof(K) + sizeof(V))));
			nmap.size = 0;
			nmap.capacity = nsize;
			nmap.firstIndex = nsize;
			
			size_t left = size;
			for (size_t i = firstIndex; i < capacity && left > 0; i++) {
				if (taken[i]) {
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
				if (taken[i]) {
					nmap.put(keys[i], values[i]);
					left--;
				}
			}

			return nmap;
		}

		void destroy() {
			if (keys) {
				allocator->free(keys, capacity * (sizeof(K) + sizeof(V) + sizeof(bool)));
				keys = nullptr;
				values = nullptr;
				taken = nullptr;
			}
			size = 0;
			capacity = 0;
			firstIndex = 0;
		}

		size_t find(const K& key) {
			if (size == 0) { return npos; }
			auto idx = get_index(key);
			bool firstTaken = taken[idx];
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) % capacity; 

				if (firstTaken && !taken[ridx]) {
					return npos;
				}

				if (taken[ridx] && keys[ridx] == key) {
					return ridx;
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
			auto idx = get_index(key);
			for (uint32_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) % capacity;

				if (taken[ridx]) {
					if (keys[ridx] == key) {
						values[ridx] = value;
						return values + ridx;
					}
				} else {
					keys[ridx] = key;
					values[ridx] = value;
					taken[ridx] = true;
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
			taken[idx] = false;
			size --;
			if (firstIndex == idx) { //calculate new first index
				firstIndex = capacity;
				for (auto i = 0u; i < capacity; i++) {
					if (taken[i]) {
						firstIndex = i;
						break;
					}
				}
			}
		}

		inline size_t get_index(const K& key) {
			return hash(key) % capacity;
		}

		struct Pair {
			K *key;
			V *value;
		};

		struct Iterator {
			Iterator& operator++() {
				do {
					idx ++;
					if (idx == map->capacity) { return *this; }
				} while (!map->taken[idx]);
				return *this;
			}	

			inline bool operator==(const Iterator& other) const { return map == other.map && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			Pair operator*() { return { map->keys + idx, map->values + idx }; }

			HashMap *map;
			size_t idx;
		};

		inline Iterator begin() { return Iterator{ this, firstIndex }; }
		inline Iterator end() { return Iterator{ this, capacity }; }

		IAllocator *allocator = nullptr;
		K *keys = nullptr;
		V *values = nullptr;
		bool *taken = nullptr;

		size_t size = 0, capacity = 0, firstIndex = 0;
	};

}
