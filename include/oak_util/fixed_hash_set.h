#pragma once

#include <cstring>
#include <cassert>
#include <limits>

#include "type_info.h"
#include "bit.h"
#include "cmp.h"
#include "hash.h"
#include "ptr.h"
#include "memory.h"

namespace oak {

	template<typename V>
	struct FixedHashSet {

		static constexpr size_t EMPTY_HASH = ~size_t{ 0 };

		typedef V value_type;

		struct Iterator {
			Iterator& operator++() {
				do {
					++idx;
				} while (idx != set->capacity && set->hashs[idx] == EMPTY_HASH);
				return *this;
			}

			inline bool operator==(const Iterator& other) const { return set == other.set && idx == other.idx; }
			inline bool operator!=(const Iterator& other) const { return !operator==(other); }
			inline V& operator*() { return set->values[idx]; }

			const FixedHashSet *set;
			int64_t idx;
		};

		void init(MemoryArena *arena, int64_t capacity_) {
			capacity = ensure_pow2(capacity_);
			auto valuesSize = capacity * ssizeof(V);
			auto hashsSize = capacity * ssizeof(size_t);
			auto totalSize = valuesSize + hashsSize;
			auto mem = allocate_from_arena(arena, totalSize, 1, alignof(void*));
			values = static_cast<V*>(mem);
			hashs = static_cast<size_t*>(add_ptr(mem, valuesSize));
			std::memset(hashs, 0xFF, hashsSize);
			firstIndex = capacity;
		}

		int64_t find(const V& value) const {
			if (size == 0) { return -1; }
			const auto h = HashFunc<V>{}(value);
			const auto idx = static_cast<int64_t>(h & static_cast<size_t>(capacity - 1));
			auto left = size;
			for (int64_t d = 0; d <= furthest && left > 0; ++d) {
				const auto ridx = (idx + d) & (capacity - 1);

				if (hashs[ridx] != EMPTY_HASH) {
					--left;
				}
				if (hashs[ridx] == h && EqualFunc<V, V>{}(values[ridx], value)) {
					return ridx;
				}
			}
			return -1;
		}

		int64_t find_hash(size_t h) const {
			if (size == 0) { return -1; }
			const auto idx = static_cast<int64_t>(h & static_cast<size_t>(capacity - 1));
			auto left = size;
			for (int64_t d = 0; d <= furthest && left > 0; ++d) {
				const auto ridx = (idx + d) & (capacity - 1);
				if (hashs[ridx] != EMPTY_HASH) {
					--left;
				}
				if (hashs[ridx] == h) {
					return ridx;
				}
			}
			return -1;
		}

		bool has(const V& value) const {
			return find(value) != -1;
		}

		V* get(const V& value) {
			auto idx = find(value);
			return idx != -1 ? value + idx : nullptr;
		}

		V* put(const V& value) {
			const auto h = HashFunc<V>{}(value);
			const int64_t idx = h & (capacity - 1);
			for (int64_t d = 0; d < capacity; ++d) {
				auto ridx = (idx + d) & (capacity - 1);

				if (hashs[ridx] != EMPTY_HASH) {
					if (hashs[ridx] == h && EqualFunc<V, V>{}(values[ridx], value)) {
						values[ridx] = value;
						return values + ridx;
					}
				} else {
					values[ridx] = value;
					hashs[ridx] = h;
					if (ridx < firstIndex) {
						firstIndex = ridx;
					}
					if (d > furthest) {
						furthest = d;
					}
					++size;

					return values + ridx;
				}
			}
			return nullptr;
		}

		void remove(int64_t idx) {
			assert(idx >= 0 && idx < capacity);
			assert(size > 0);
			hashs[idx] = EMPTY_HASH;
			--size;
			if (firstIndex == idx) { //calculate new first index
				firstIndex = capacity;
				for (int64_t i = 0; i < capacity; ++i) {
					if (hashs[i] != EMPTY_HASH) {
						firstIndex = i;
						break;
					}
				}
			}
		}

		inline Iterator begin() const { return Iterator{ this, firstIndex }; }
		inline Iterator end() const { return Iterator{ this, capacity }; }

		V *values = nullptr;
		size_t *hashs = nullptr;

		int64_t size = 0, capacity = 0, firstIndex = 0, furthest = 0;
	};

}

