#pragma once

#include <cassert>

#include "type_info.h"
#include "bit.h"
#include "cmp.h"
#include "hash.h"
#include "ptr.h"
#include "memory.h"

namespace oak {

	template<typename K, typename V>
	struct FixedHashMap {

		using keyType = K;
		using valueType = V;

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

			const FixedHashMap *map;
			int64_t idx;
		};

		void init(MemoryArena *arena, int64_t capacity_) {
			capacity = ensure_pow2(capacity_);
			auto keysSize = capacity * ssizeof(K);
			auto valuesSize = capacity * ssizeof(V);
			auto hashsSize = capacity * ssizeof(size_t);
			auto totalSize = keysSize + valuesSize + hashsSize;
			auto mem = allocate_from_arena(arena, totalSize, 1, 1);
			keys = static_cast<K*>(mem);
			values = static_cast<V*>(add_ptr(mem, keysSize));
			hashs = static_cast<size_t*>(add_ptr(mem, keysSize + valuesSize));
			for (int64_t i = 0; i < capacity; i++) {
				hashs[i] = EMPTY_HASH;
			}
		}

		int64_t find(const K& key) const {
			assert(keys);
			if (size == 0) { return -1; }
			const auto h = HashFunc<K>{}(key);
			const int64_t idx = h & (capacity - 1);
			auto left = size;
			for (int64_t d = 0; d <= furthest && left > 0; d++) {
				const auto ridx = (idx + d) & (capacity - 1);
				if (hashs[ridx] != EMPTY_HASH) {
					left--;
				}
				if (hashs[ridx] == h && CmpFunc<K, K>{}(keys[ridx], key)) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_hash(size_t h) const {
			if (size == 0) { return -1; }
			const int64_t idx = h & (capacity - 1);
			auto left = size;
			for (int64_t d = 0; d <= furthest && left > 0; d++) {
				const auto ridx = (idx + d) & (capacity - 1);
				if (hashs[ridx] != EMPTY_HASH) {
					left--;
				}
				if (hashs[ridx] == h) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_value(const V& value) const {
			auto left = size;
			for (int64_t i = firstIndex; i < capacity && left > 0; i++) {
				if (hashs[i] != EMPTY_HASH) {
					left--;
					if (CmpFunc<V, V>{}(values[i], value)) {
						return i;
					}
				}
			}
			return -1;
		}

		bool has(const K& key) const {
			return find(key) != -1;
		}

		V* get(const K& key) {
			auto idx = find(key);
			return idx != -1 ? values + idx : nullptr;
		}

		V* put(const K& key, const V& value) {
			const auto h = HashFunc<K>{}(key);
			const int64_t idx = h & (capacity - 1);
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
					if (d > furthest) {
						furthest = d;
					}
					size++;

					return values + ridx;
				}
			}
			return nullptr;
		}

		void remove(int64_t idx) {
			assert(keys);
			assert(idx >= 0 && idx < capacity);
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

		K *keys = nullptr;
		V *values = nullptr;
		size_t *hashs = nullptr;

		int64_t size = 0, capacity = 0, firstIndex = 0, furthest = 0;
	};

	template<typename V>
	struct IntHashMap {

		using valueType = V;

		struct Pair {
			size_t *key;
			V *value;
		};

		static constexpr size_t NULL_KEY = ~size_t{ 0 };

		struct Iterator {
			Iterator& operator++() {
				do {
					idx ++;
					if (idx == map->capacity) { return *this; }
				} while (map->keys[idx] == NULL_KEY);
				return *this;
			}

			inline bool operator==(const Iterator& other) const { return map == other.map && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			Pair operator*() { return { map->keys + idx, map->values + idx }; }

			const IntHashMap *map;
			int64_t idx;
		};

		void init(MemoryArena *arena, int64_t capacity_) {
			capacity = ensure_pow2(capacity_);
			auto keysSize = capacity * ssizeof(size_t);
			auto valuesSize = capacity * ssizeof(V);
			auto totalSize = keysSize + valuesSize;
			auto mem = allocate_from_arena(arena, totalSize, 1, 1);
			keys = static_cast<size_t*>(mem);
			values = static_cast<V*>(add_ptr(mem, keysSize));
			for (int64_t i = 0; i < capacity; i++) {
				keys[i] = NULL_KEY;
			}
		}

		int64_t find(const size_t key) const {
			assert(keys);
			if (size == 0) { return -1; }
			const int64_t idx = key & (capacity - 1);
			auto left = size;
			for (int64_t d = 0; d <= furthest && left > 0; d++) {
				const auto ridx = (idx + d) & (capacity - 1);
				if (keys[ridx] != NULL_KEY) {
					left--;
				}
				if (keys[ridx] == key) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_value(const V& value) const {
			auto left = size;
			for (int64_t i = firstIndex; i < capacity && left > 0; i++) {
				if (keys[i] != NULL_KEY) {
					left--;
					if (CmpFunc<V, V>{}(values[i], value)) {
						return i;
					}
				}
			}
			return -1;
		}

		bool has(const size_t key) const {
			return find(key) != -1;
		}

		V* get(const size_t key) {
			auto idx = find(key);
			return idx != -1 ? values + idx : nullptr;
		}

		V* put(const size_t key, const V& value) {
			const int64_t idx = key & (capacity - 1);
			for (int64_t d = 0; d < capacity; d++) {
				auto ridx = (idx + d) & (capacity - 1);

				if (keys[ridx] != NULL_KEY) {
					if (keys[ridx] == key) {
						values[ridx] = value;
						return values + ridx;
					}
				} else {
					keys[ridx] = key;
					values[ridx] = value;
					if (ridx < firstIndex) {
						firstIndex = ridx;
					}
					if (d > furthest) {
						furthest = d;
					}
					size++;

					return values + ridx;
				}
			}
			return nullptr;
		}

		void remove(int64_t idx) {
			assert(keys);
			assert(idx >= 0 && idx < capacity);
			assert(size > 0);
			keys[idx] = NULL_KEY;
			size--;
			if (firstIndex == idx) { //calculate new first index
				firstIndex = capacity;
				for (int64_t i = 0; i < capacity; i++) {
					if (keys[i] != NULL_KEY) {
						firstIndex = i;
						break;
					}
				}
			}
		}

		inline Iterator begin() const { return Iterator{ this, firstIndex }; }
		inline Iterator end() const { return Iterator{ this, capacity }; }

		size_t *keys = nullptr;
		V *values = nullptr;

		int64_t size = 0, capacity = 0, firstIndex = 0, furthest = 0;
	};

}

