#pragma once

#include <cstring>

#include "types.h"
#include "bit.h"
#include "memory.h"

namespace oak {

	template<typename... types>
	struct SOA {
		void *data = nullptr;
		i64 count = 0;

		constexpr std::tuple<types&...> operator[](i64 const index) const noexcept;
	};

	template<size_t index, typename... types>
	constexpr typename std::tuple_element<index, SOA<types...>>::type get(SOA<types...> const& soa) noexcept;

	template<typename T>
	struct Vector : Slice<T> {


		using Slice<T>::data;
		using Slice<T>::count;

		i64 capacity = 0;

		constexpr Vector() noexcept = default;

		Vector(Allocator *allocator, T *data_, i64 count_) noexcept : Slice<T>{}, capacity{ 0 } {
			resize(allocator, count_);
			std::memcpy(data, data_, count_ * sizeof(T));
		}

		template<int C>
		Vector(Allocator *allocator, T const (&array)[C]) noexcept : Slice<T>{}, capacity{ 0 } {
			resize(allocator, C);
			std::memcpy(data, array, C * sizeof(T));
		}

		Vector(Allocator *allocator, std::initializer_list<T> list) noexcept : Slice<T>{}, capacity{ 0 } {
			resize(allocator, list.size());
			auto iter = std::begin(list);
			for (i64 i = 0; i < count; ++i) {
				data[i] = *iter;
				++iter;
			}
		}


		void reserve(Allocator *allocator, i64 nCapacity) noexcept {
			if (nCapacity <= capacity) {
				// If the array is already big enough no need to resize
				return;
			}
			nCapacity = ensure_pow2(nCapacity);
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

		Vector clone(Allocator *allocator) const noexcept {
			Vector nVec;
			nVec.reserve(allocator, capacity);
			nVec.count = count;
			std::memcpy(nVec.data, data, count * sizeof(T));
			return nVec;
		}

		void destroy(Allocator *allocator) noexcept {
			if (data) {
				deallocate(allocator, data, capacity * sizeof(T));
				data = nullptr;
			}
			count = 0;
			capacity = 0;
		}

		T* push(Allocator *allocator, T const& value) noexcept {
			if (count == capacity) {
				reserve(allocator, capacity == 0 ? 4 : capacity * 2);
			}
			data[count++] = value;
			return data + count - 1;
		}

		T* insert(Allocator *allocator, T const& value, i64 idx) noexcept {
			if (idx == -1 || idx == count) {
				return push(allocator, value);
			}
			resize(allocator, count + 1);
			std::memmove(data + idx + 1, data + idx, (count - 1 - idx) * sizeof(T));
			data[idx] = value;
			return data + idx;
		}

		constexpr void remove(i64 idx) noexcept {
			assert(0 <= idx && idx < count);
			// Swap and pop
			data[idx] = data[--count];
		}

		void remove_ordered(i64 idx) noexcept {
			assert(0 <= idx && idx < count);
			// Move the elements after idx down one index
			std::memmove(data + idx, data + idx + 1, (count - 1 - idx) * sizeof(T));
			--count;
		}

		constexpr void clear() noexcept {
			count = 0;
		}
	};


	template<typename Key, typename HFn = HashFn<Key>, typename CFn = CmpFn<Key, Key>, typename... Values>
	struct HashSet : HFn, CFn {

		struct Iterator {
			Iterator& operator++() noexcept {
				do {
					++idx;
				} while (idx != set->data.count && set->is_empty(idx));
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
			i64 idx = 0;
		};

		SOA<bool, Key, Values...> data;
		i64 firstIndex = 0;

		void init(Allocator *allocator, i64 capacity_) noexcept {
			// Allocate storage
			data.count = ensure_pow2(capacity_);
			data.data = allocate_soa<bool, Key, Values...>(allocator, data.count);
			assert(data.data);

			// Initilize empty array
			for (auto& elem : get<0>(data)) {
				elem = true;
			}

			firstIndex = data.count;
		}

		void destroy(Allocator *allocator) noexcept {
			deallocate_soa<bool, Key, Values...>(allocator, data.count);
			data = {};
		}

		constexpr bool is_empty(i64 const idx) const noexcept {
			return get<0>(data)[idx];
		}

		constexpr u64 hash(Key const& key) const noexcept {
			return HFn::operator()(key);
		}

		constexpr i64 cmp(Key const& lhs, Key const& rhs) const noexcept {
			return CFn::operator()(lhs, rhs);
		}

		constexpr i64 slot(u64 const hash) const noexcept {
			return hash & (data.count - 1);
		}

		constexpr i64 first_index() const noexcept {
			i64 idx = 0;
			for (auto empty : get<0>(data)) {
				if (!empty) { break; }
				++idx;
			}
			return idx;
		}

		constexpr i64 find(Key const& key) const noexcept {
			auto const& keys = get<1>(data);
			auto const idx = slot(hash(key));
			for (i64 d = 0; d < data.count; ++d) {
				auto ridx = (idx + d) & (data.count - 1);
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
			auto const& keys = get<1>(data);
			auto const idx = slot(hash(key));
			for (i64 d = 0; d < data.count; ++d) {
				auto ridx = (idx + d) & (data.count - 1);

				if (is_empty(ridx) || cmp(keys[ridx], key) == 0) {
					data[ridx] = std::make_tuple(false, key, values...);
					if (ridx < firstIndex) {
						firstIndex = ridx;
					}
					return ridx;
				}

			}
			return -1;
		}

		constexpr void remove(i64 const idx) noexcept {
			assert(!is_empty(idx));
			// cidx is the index of the slot we are trying to empty
			// ridx is the index of the slot we are looking at to try and fill it
			auto const &keys = get<1>(data);
			auto cidx = idx;
			for (i64 d = 1; d < data.count; ++d) {
				auto ridx = (idx + d) & (data.count - 1);
				if (is_empty(ridx)) {
					break;
				}
				// If the element in the slot we are looking at belongs at an earlier slot
				auto const scidx = slot(hash(keys[cidx]));
				auto const sridx = slot(hash(keys[ridx]));
				if ((ridx > cidx || sridx > ridx) && sridx <= scidx) {
					data[cidx] = data[ridx];
					cidx = ridx;
				}
			}
			get<0>(data)[cidx] = true;
			firstIndex = first_index();
		}

		constexpr void clear() noexcept {
			// Set all slots to empty, effectively removing all elements
			for (auto& elem : get<0>(data)) {
				elem = true;
			}
			firstIndex = data.count;
		}

		constexpr Iterator begin() const noexcept {
			return Iterator{ this, firstIndex };
		}

		constexpr Iterator end() const noexcept {
			return Iterator{ this, data.count };
		}

	};

	template<typename K, typename V>
	using HashMap = HashSet<K, HashFn<K>, CmpFn<K, K>, V>;

}

namespace std {

	template<typename... types>
	struct tuple_size<oak::SOA<types...>> {
		static constexpr size_t value = sizeof...(types);
	};

	template<size_t index, typename... types>
	struct tuple_element<index, oak::SOA<types...>> {
		using type = oak::Slice<typename tuple_element<index, tuple<types...>>::type>;
	};

}

namespace oak {

	template<size_t index, typename... types>
	constexpr typename std::tuple_element<index, SOA<types...>>::type get(SOA<types...> const& soa) noexcept {
		using type = typename std::tuple_element<index, std::tuple<types...>>::type;

		auto offset = soa_offset<index, types...>(soa.count);
		offset = align_int(offset, alignof(type));

		return { static_cast<type*>(add_ptr(soa.data, offset)), soa.count };
	}

	namespace detail {

		template<typename tupleT, typename soaT, size_t... ints>
		constexpr tupleT soa_sub_script_impl(soaT const& soa, i64 const index, std::index_sequence<ints...>) noexcept {
			return { get<ints>(soa)[index]..., };
		}

	}

	template<typename... types>
	constexpr std::tuple<types&...> SOA<types...>::operator[](i64 const index) const noexcept {
		using indices = std::make_index_sequence<std::tuple_size<std::tuple<types...>>::value>;
		return detail::soa_sub_script_impl<std::tuple<types&...>>(*this, index, indices{});
	}

}

