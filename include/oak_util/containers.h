#pragma once

#include <cstring>

#include "types.h"
#include "bit.h"
#include "memory.h"

namespace oak {

	template<typename... types>
	struct SOA : std::tuple<types*...> {

		using Base = std::tuple<types*...>;

		template<usize... ints>
		void init_impl(void *const data, i64 const count, std::index_sequence<ints...>) noexcept {
			((void)(std::get<ints>(*this) = static_cast<types*>(add_ptr(data, soa_offset<ints, types...>(count)))), ...);
		}

		void init(Allocator *const allocator, i64 const count) noexcept {
			auto data = allocate_soa<types...>(allocator, count);

			using indices = std::make_index_sequence<std::tuple_size<std::tuple<types...>>::value>;
			init_impl(data, count, indices{});
		}

		void destroy(Allocator *const allocator, i64 const count) noexcept {
			deallocate_soa<types...>(allocator, std::get<0>(*this), count);
		}

		constexpr std::tuple<types&...> operator[](i64 const index) const noexcept;
	};

	template<usize index, typename... types>
	constexpr typename std::tuple_element<index, SOA<types...>>::type get(SOA<types...> const& soa) noexcept;

	template<typename T>
	struct _reflect(array) FixedVector {

		using ElemType = T;

		_reflect() T *data = nullptr;
		_reflect() i64 capacity = 0;

		constexpr FixedVector() noexcept = default;

		FixedVector(Allocator *allocator, T const *data_, i64 count_) noexcept
				: data{ nullptr }, capacity{ 0 } {
			reserve(allocator, count_);
			std::memcpy(data, data_, count_ * sizeof(T));
		}

		template<i64 Count>
		FixedVector(Allocator *allocator, T const (&array)[Count]) noexcept
				: data{ nullptr }, capacity{ 0 } {
			reserve(allocator, Count);
			std::memcpy(data, array, Count * sizeof(T));
		}

		FixedVector(Allocator *allocator, std::initializer_list<T> list) noexcept
				: data{ nullptr }, capacity{ 0 } {
			reserve(allocator, list.size());
			std::memcpy(data, list.data(), list.size() * sizeof(T));
		}

		void reserve(Allocator *allocator, i64 nCapacity) noexcept {
			if (nCapacity <= capacity) {
				// If the array is already big enough no need to resize
				return;
			}
			auto nData = allocate<T>(allocator, nCapacity);
			if (data) {
				std::memcpy(nData, data, capacity * sizeof(T));
				deallocate(allocator, data, capacity);
			}
			data = nData;
			capacity = nCapacity;
		}

		FixedVector clone(Allocator *allocator, i64 minCapacity = 0) const noexcept {
			FixedVector nVec;
			nVec.reserve(allocator, capacity < minCapacity ? minCapacity : capacity);
			std::memcpy(nVec.data, data, capacity * sizeof(T));
			return nVec;
		}

		void destroy(Allocator *allocator) noexcept {
			if (data) {
				deallocate(allocator, data, capacity);
				data = nullptr;
			}
			capacity = 0;
		}

		constexpr T* begin() noexcept {
			return data;
		}

		constexpr T* end() noexcept {
			return data + capacity;
		}

		constexpr T const* begin() const noexcept {
			return data;
		}

		constexpr T const* end() const noexcept {
			return data + capacity;
		}

		constexpr T& operator[](i64 const idx) noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr T const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, capacity };
		}

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, capacity };
		}

	};

	template<typename T>
	struct _reflect(array) Vector {

		using ElemType = T;

		_reflect() T *data = nullptr;
		_reflect() i64 count = 0;
		_reflect() i64 capacity = 0;

		constexpr Vector() noexcept = default;

		Vector(Allocator *allocator, T const *data_, i64 count_) noexcept
				: data{ nullptr }, count{ 0 }, capacity{ 0 } {
			resize(allocator, count_);
			std::memcpy(data, data_, count_ * sizeof(T));
		}

		template<i64 Count>
		Vector(Allocator *allocator, T const (&array)[Count]) noexcept
				: data{ nullptr }, count{ 0 }, capacity{ 0 } {
			resize(allocator, Count);
			std::memcpy(data, array, Count * sizeof(T));
		}

		Vector(Allocator *allocator, std::initializer_list<T> list) noexcept
				: data{ nullptr }, count{ 0 }, capacity{ 0 } {
			resize(allocator, list.size());
			std::memcpy(data, list.data(), list.size() * sizeof(T));
		}

		void reserve(Allocator *allocator, i64 nCapacity) noexcept {
			if (nCapacity <= capacity) {
				// If the array is already big enough no need to resize
				return;
			}
			auto nData = allocate<T>(allocator, nCapacity);
			if (data) {
				std::memcpy(nData, data, count * sizeof(T));
				deallocate(allocator, data, capacity);
			}
			data = nData;
			capacity = nCapacity;
		}

		void resize(Allocator *allocator, i64 nCount) noexcept {
			reserve(allocator, nCount);
			count = nCount;
		}

		Vector clone(Allocator *allocator, i64 minCapacity = 0) const noexcept {
			Vector nVec;
			nVec.reserve(allocator, capacity < minCapacity ? minCapacity : capacity);
			nVec.count = count;
			std::memcpy(nVec.data, data, count * sizeof(T));
			return nVec;
		}

		void destroy(Allocator *allocator) noexcept {
			if (data) {
				deallocate(allocator, data, capacity);
				data = nullptr;
			}
			count = 0;
			capacity = 0;
		}

		constexpr void clear() noexcept {
			count = 0;
		}

		constexpr T* begin() noexcept {
			return data;
		}

		constexpr T* end() noexcept {
			return data + count;
		}

		constexpr T const* begin() const noexcept {
			return data;
		}

		constexpr T const* end() const noexcept {
			return data + count;
		}

		constexpr T& operator[](i64 const idx) noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr T const& operator[](i64 const idx) const noexcept {
			assert(0 <= idx && idx < capacity);
			return data[idx];
		}

		constexpr operator Slice<T>() noexcept {
			return Slice<T>{ data, count };
		}

		constexpr operator Slice<T const>() const noexcept {
			return Slice<T const>{ data, count };
		}

	};


	/*
u64 hash(f32 value) {
    auto bytes = reinterpret_cast<char*>(&value);

    u64 hash = 0;
    // Is this even a hash function lulz
    for (u64 i = 0; i < sizeof(value); ++i) {
        hash += bytes[i];
        hash *= 65593;
        hash = (hash << 35 | hash >> 29);
    }

    return hash;

}

struct DenseHashSet {
    f32 *values = nullptr;
    i32 *dense = nullptr;
    i32 *sparse = nullptr;

    i64 count = 0;
    i64 capacity = 0;

    bool exists(i32 sparseIdx) const {
        assert(sparseIdx < capacity);
        auto denseIdx = sparse[sparseIdx];
        return denseIdx < count && dense[denseIdx] == sparseIdx;
    }

    i64 insert(f32 value) {
        // Sparse index
        auto sIdx = static_cast<i64>(hash(value) & (capacity - 1));

        for (i64 d = 0; d < capacity; ++d) {
            auto rsIdx = (sIdx + d) & (capacity - 1);
            if (!exists(rsIdx)) {
                // We've found an empty spot
                auto denseIdx = count;
                dense[denseIdx] = rsIdx;
                sparse[rsIdx] = denseIdx;
                values[count++] = value;
                return denseIdx;

            }
            if (values[sparse[rsIdx]] == value) {
                return sparse[rsIdx];
            }
        }

        return -1;

    }

    i64 find(f32 value) const {
        // Sparse index
        auto sIdx = static_cast<i64>(hash(value) & (capacity - 1));

        for (i64 d = 0; d < capacity; ++d) {
            auto rsIdx = (sIdx + d) & (capacity - 1);
            if (exists(rsIdx) && values[sparse[rsIdx]] == value) {
                return sparse[rsIdx];
            }
        }

        return -1;

    }

    void remove(i64 idx) {
        assert(exists(idx));

        // cidx is the index of the slot we are trying to empty
        // ridx is the index of the slot we are looking at to try and fill it
        auto cidx = idx;
        auto scidx = static_cast<i64>(hash(values[sparse[cidx]]) & (capacity - 1));

        // Remove element from dense array
        auto denseIdx = sparse[idx];
        auto last = --count;
        values[denseIdx] = values[last];
        dense[denseIdx] = dense[last];
        sparse[dense[denseIdx]] = denseIdx;

        for (i64 d = 1; d < capacity; ++d) {
            auto ridx = (idx + d) & (capacity - 1);
            if (!exists(ridx)) {
                break;
            }
            // If the element in the slot we are looking at belongs at an earlier slot
            auto const sridx = static_cast<i64>(hash(values[sparse[ridx]]) & (capacity - 1));
            if ((ridx > cidx || sridx > ridx) && sridx <= scidx) {
                sparse[cidx] = sparse[ridx];
                dense[sparse[cidx]] = cidx;

                cidx = ridx;
                scidx = sridx;
            }
        }

    }

    bool is_valid() {
        // Check for valid dense to sparse map
        for (i32 i = 0; i < count; ++i) {
            if (auto sidx = dense[i]; sidx < 0 || sidx >= capacity || i != sparse[sidx]
                || !exists(sidx) || sidx != dense[sparse[sidx]] ) {
                std::fprintf(stderr, "mapping invalid\n");
                return false;
            }
        }
        // Check for valid linearity
        for (i32 i = 0; i < count; ++i) {
            auto rsidx = dense[i];
            auto sidx = static_cast<i64>(hash(values[i]) & (capacity - 1));
            for (auto it = rsidx; it != sidx;) {
                if (!exists(it)) {
                    std::fprintf(stderr, "linearity invalid\n");
                    return false;
                }
                --it;
                if (it < 0)
                    it += capacity;
                it = it & (capacity - 1);
            }
        }
        return true;
    }

    void dump() {
        std::fprintf(stderr, "[ ");
        for (i32 i = 0; i < capacity; ++i) {
                std::fprintf(stderr, "%i <-> %i: %f, \n", dense[sparse[i]], sparse[i], values[sparse[i]]);
        }
        std::fprintf(stderr, " ]\n");
    }

};

int main(int, char **) {
    DenseHashSet set;
    set.capacity = 32;
    set.count = 0;
    set.values = static_cast<f32*>(std::calloc(set.capacity, sizeof(f32)));
    set.dense = static_cast<i32*>(std::calloc(set.capacity, sizeof(i32)));
    set.sparse = static_cast<i32*>(std::calloc(set.capacity, sizeof(i32)));

    set.insert(29);

    assert(set.is_valid());
    assert(set.count == 1);
    assert(set.find(29) != -1);

    set.insert(29);

    assert(set.is_valid());
    assert(set.count == 1);
    assert(set.find(29) != -1);

    set.insert(41);

    assert(set.is_valid());
    assert(set.count == 2);
    assert(set.find(29) != -1);
    assert(set.find(41) != -1);

    for (i32 i = 0; i < set.count;) {
        set.remove(set.dense[set.find(set.values[i])]);
        assert(set.is_valid());
    }

    for (i32 i = 0; i < set.capacity; ++i) {
        auto idx = set.insert(static_cast<f32>(std::rand()) / RAND_MAX);
        if (idx == -1)
            std::fprintf(stderr, "failed to insert value\n");
    }

    set.dump();

    assert(set.is_valid());

    while (set.count > 0) {
        auto i = std::rand() % set.count;
        std::fprintf(stderr, "value: %f, count: %li\n", set.values[i], set.count);
        set.remove(set.dense[set.find(set.values[i])]);
        assert(set.is_valid());
    }


}


*/



	template<typename Key, typename HFn = HashFn<Key>, typename CFn = CmpFn<Key, Key>, typename... Values>
	struct HashSet : HFn, CFn {

		struct Iterator {
			Iterator& operator++() noexcept {
				do {
					++idx;
				} while (idx != set->capacity && empty[idx]);
				return *this;
			}

			constexpr bool operator==(Iterator const& other) const noexcept {
				return set == other.set && idx == other.idx;
			}

			constexpr bool operator!=(Iterator const& other) const noexcept {
				return !operator==(other);
			}

			constexpr auto operator*() const noexcept {
				return set->data[idx];
			}

			HashSet const *set = nullptr;
			bool *empty = nullptr;
			i64 idx = 0;
		};

		SOA<bool, Key, Values...> data;
		i64 count = 0;
		i64 capacity = 0;
		i64 firstIndex = 0;

		void init(Allocator *const allocator, i64 const capacity_) noexcept {
			// Allocate storage
			count = 0;
			capacity = ensure_pow2(capacity_);
			data.init(allocator, capacity);

			// Initilize empty array
			for (auto& elem : Slice{ std::get<0>(data), capacity }) {
				elem = true;
			}

			firstIndex = capacity;
		}

		void destroy(Allocator *const allocator) noexcept {
			data.destroy(allocator, capacity);
			data = {};
			count = 0;
			capacity = 0;
			firstIndex = 0;
		}

		constexpr bool is_empty(i64 const idx) const noexcept {
			assert(0 <= idx && idx < capacity);
			return std::get<0>(data)[idx];
		}

		constexpr u64 hash(Key const& key) const noexcept {
			return HFn::operator()(key);
		}

		constexpr i64 cmp(Key const& lhs, Key const& rhs) const noexcept {
			return CFn::operator()(lhs, rhs);
		}

		constexpr i64 slot(u64 const hash) const noexcept {
			return hash & (capacity - 1);
		}

		constexpr i64 first_index() const noexcept {
			i64 idx = 0;
			for (auto empty : Slice{ std::get<0>(data), capacity }) {
				if (!empty)
					break;
				++idx;
			}
			return idx;
		}

		constexpr i64 find(Key const& key) const noexcept {
			auto const keys = std::get<1>(data);
			auto const idx = slot(hash(key));
			for (i64 d = 0; d < capacity; ++d) {
				auto ridx = (idx + d) & (capacity - 1);
				// If we reach an empty cell then exit because the key is not in the table
				if (is_empty(ridx)) {
					return -1;
				}
				// We found the key so return
				if (cmp(keys[ridx], key) == 0) {
					return ridx;
				}
			}
			return -1;
		}

		constexpr i64 insert(Key const& key, Values const&... values) noexcept {
			auto const keys = std::get<1>(data);
			auto const idx = slot(hash(key));
			for (i64 d = 0; d < capacity; ++d) {
				auto ridx = (idx + d) & (capacity - 1);

				if (auto isEmpty = is_empty(ridx); isEmpty || cmp(keys[ridx], key) == 0) {
					data[ridx] = std::make_tuple(false, key, values...);
					if (ridx < firstIndex) {
						firstIndex = ridx;
					}
					count += isEmpty;
					return ridx;
				}

			}
			return -1;
		}

		constexpr void remove(i64 const idx) noexcept {
			assert(!is_empty(idx));
			// cidx is the index of the slot we are trying to empty
			// ridx is the index of the slot we are looking at to try and fill it
			auto empty = std::get<0>(data);
			auto const keys = std::get<1>(data);
			auto cidx = idx;
			for (i64 d = 1; d < capacity; ++d) {
				auto ridx = (idx + d) & (capacity - 1);
				if (is_empty(ridx)) {
					break;
				}
				// If the element in the slot we are looking at belongs at an earlier slot
				auto const sridx = slot(hash(keys[ridx]));
				if (cidx == sridx || ((sridx - cidx) & capacity >> 1) != 0) {
					data[cidx] = data[ridx];
					cidx = ridx;
				}
			}
			empty[cidx] = true;
			firstIndex = first_index();
			--count;
		}

		constexpr void clear() noexcept {
			// Set all slots to empty, effectively removing all elements
			for (auto& elem : Slice{ std::get<0>(data), capacity }) {
				elem = true;
			}
			count = 0;
			firstIndex = capacity;
		}

		constexpr Iterator begin() const noexcept {
			return Iterator{ this, std::get<0>(data), firstIndex };
		}

		constexpr Iterator end() const noexcept {
			return Iterator{ this, std::get<0>(data), capacity };
		}

		template<typename T>
		constexpr T const& get(Key const& key) const noexcept {
			auto idx = find(key);
			assert(idx != -1);
			return std::get<T*>(data)[idx];
		}

		template<typename T>
		constexpr T& get(Key const& key) noexcept {
			auto idx = find(key);
			assert(idx != -1);
			return std::get<T*>(data)[idx];
		}

	};

	template<typename K, typename V>
	using HashMap = HashSet<K, HashFn<K>, CmpFn<K, K>, V>;

	namespace detail {

		template<typename tupleT, typename soaT, usize... ints>
		constexpr tupleT soa_sub_script_impl(soaT const& soa, i64 const index, std::index_sequence<ints...>) noexcept {
			return { std::get<ints>(soa)[index]..., };
		}

	}

	template<typename... types>
	constexpr std::tuple<types&...> SOA<types...>::operator[](i64 const index) const noexcept {
		using indices = std::make_index_sequence<std::tuple_size<std::tuple<types...>>::value>;
		return detail::soa_sub_script_impl<std::tuple<types&...>>(*this, index, indices{});
	}

}

